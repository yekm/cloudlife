/* marbling, Copyright © 2021-2022 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * This generates a random field with Perlin Noise, then permutes it with
 * Fractal Brownian Motion to create images that somewhat resemble clouds,
 * or the striations in marble, depending on the parameters selected and
 * the colors chosen.
 *
 * Perlin Noise, SIGGRAPH 2002:
 *
 *     https://mrl.cs.nyu.edu/~perlin/noise/
 *     https://mrl.cs.nyu.edu/~perlin/paper445.pdf
 *     https://en.wikipedia.org/wiki/Perlin_noise
 *
 * Fractal Brownian Motion:
 *
 *     https://en.wikipedia.org/wiki/Fractional_Brownian_motion
 *     https://thebookofshaders.com/13/
 *     https://www.shadertoy.com/view/Msf3WH
 *
 * These algorithms lend themselves well to SIMD supercomputers, which is to
 * say GPUs.  Ideally, this program would be written in Shader Language, but
 * XScreenSaver still targets OpenGL systems that don't support GLSL, so we
 * are doing the crazy thing here of trying to run this highly parallelizable
 * algorithm on the CPU instead of the GPU.  This sort-of works out because
 * modern CPUs have a fair amount of parallel-computation features on their
 * side of the fence as well.  (Generally speaking, your CPU is a Cray and
 * your GPU is a Connection Machine, except that your phone does not
 * typically require liquid nitrogen cooling and a dedicated power plant).
 *
 * Initial version by jwz; black magic for pthreads and CPU-specific vector
 * ops added by Dave Odell <dmo2118@gmail.com>.  Here be parallel monsters.
 */

#include "marbling.hpp"
#include "concurrency/parallel_batch.hpp"
#include "imgui.h"
#include "imgui_elements.h"
#include "random.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <thread>
#include <vector>

namespace {
#if defined __GNUC__ || defined __clang__ || \
  defined __STDC_VERSION__ && __STDC_VERSION__ > 199901L
# define INLINE inline
#else
# define INLINE
#endif

/* Use GCC/Clang's vector SIMD extensions, when possible.
   https://gcc.gnu.org/onlinedocs/gcc/Vector-Extensions.html
 */

#if defined __GNUC__ || defined __clang__
# if defined __x86_64__ || defined __i386__
/* 32-bit x86 doesn't usually have SSE2 enabled in the compiler by default,
   so on that platform this will normally be really slow.

   This doesn't use emmintrin.h/avx2intrin.h because the intrinsics in those
   headers use __m128i or __m256i, which aren't strongly typed, and don't work
   with GCC vector arithmetic.
 */
#  if defined __AVX2__
#   define VSIZE 16
#   define IMUL_HI_OP __builtin_ia32_pmulhw256
#   define MUL_HI_OP __builtin_ia32_pmulhuw256
#  elif defined __SSE2__
#   define VSIZE 8
#   define IMUL_HI_OP __builtin_ia32_pmulhw128
#   define MUL_HI_OP __builtin_ia32_pmulhuw128
#  endif

#  ifdef VSIZE
typedef uint16_t v_uhi __attribute__((vector_size(VSIZE * 2)));
typedef int16_t v_hi __attribute__((vector_size(VSIZE * 2)));

static INLINE v_hi IMUL_HI(v_hi a, v_hi b) { return IMUL_HI_OP(a, b); }
static INLINE v_uhi MUL_HI(v_uhi a, v_uhi b) { return (v_uhi)MUL_HI_OP((v_hi)a, (v_hi)b); }

#  endif
# endif


# if defined __ARM_NEON
#  include <arm_neon.h>
#  define VSIZE 8

/* No unsigned vector multiply on ARM NEON. Closest is vqdmulhq, a.k.a.
   "Vector Saturating Doubling Multiply Returning High Half". (Say that five
   times fast.) The 'doubling' part comes from how in signed multiplication
   the most significant bit always matches the sign bit, so ARM shifts the
   result left by one, preserving an extra bit of precision.

   Which is basically:
   int16_t vqdmulhq_s16(int16_t a, int16_t b)
   {
     return ((int32_t)a * b) >> 15;
   }

   Google has NEON support turned on by default for recent NDK releases.
   Android doesn't require NEON, but it's apparently very uncommon for an ARM
   Android device to not have NEON.

   Apparently iPhones got NEON starting with the 3GS, back in 2009.
 */

typedef uint16x8_t v_uhi;
typedef int16x8_t v_hi;

# endif
#endif

#ifndef VSIZE
# define VSIZE 1
typedef uint16_t v_uhi;
typedef int16_t v_hi;
# define MUL_HI(a, b) (((uint32_t)(uint16_t)(a) * (uint16_t)(b)) >> 16)
# define IMUL_HI(a, b) (((int32_t)(int16_t)(a) * (int16_t)(b)) >> 16)
# define VEC_INDEX(v, i) (v)
#else
# define VEC_INDEX(v, i) ((v)[i])
#endif

/* Perlin Noise
 */

const unsigned noise_work_bits = 13; /* Max: 13 */

#if __ARM_NEON
const unsigned lerp_loss = 1;
#else
const unsigned lerp_loss = 2; /* Min: 2 */
#endif

/* == 8 for x86 and scalar. (Nice.) */
/* const unsigned noise_out_bits = noise_work_bits - 3 * lerp_loss + 1; */
#define noise_out_bits ((noise_work_bits - 3) * lerp_loss + 1)
const unsigned noise_in_bits = 8;

static INLINE v_uhi
broadcast (int16_t x)
{
#if VSIZE == 1
  v_hi r = x;
#else
  v_hi r = {0};
  r = r + x;
#endif
  return (v_uhi)r;
}

static INLINE v_uhi
fade (v_uhi t)
{
  const uint16_t F = 256;

  /* This whole thing is playing fast and loose with signdedness, mostly
     because __builtin_ia32_pmulhuw256 is an unsigned op that requires signed
     params, and Android's clang doesn't seem to care about signed vs.
     unsigned vector variables.

     The multiplications below should be unsigned, but the result is the same
     either way, because it's getting the low 16 bits (not the high 16).

     All the v_(u)hi variables are vectors of 16-bit fixed-point ints, maybe
     signed, maybe unsigned.  noise_in_bits and noise_out_bits determine how
     many fractional bits are in use for the v_uhi variables that go into/come
     out of noise().  And noise_out_bits is governed by the big expression at
     the end of noise():

     1. Going in, there's noise_work_bits worth of fraction in c0-c7.
     2. The multiply in LERP() (either vqdmulhq_s16 or IMUL_HI) shifts
        lerp_loss bits of fraction off the end of each int16. Because
        LERP() is nested three deep, it's 3 * lerp_loss.
     3. SCALE() shifts things one bit in the other direction.
        Hence, noise_work_bits - 3 * lerp_loss + 1.
   */
#if __ARM_NEON
  v_uhi ut2 = (t * t) >> 1;
  v_hi it2 = (v_hi)ut2;
  v_hi it = (v_hi)t;
  v_hi iret =
    vqdmulhq_s16(it2, 10 * it - vqdmulhq_s16(it2, (int16_t)(15 * F) - it * 6)) <<
    (noise_work_bits - noise_in_bits + 1);
  return (v_uhi)iret;
#else
  v_uhi t2 = t * t;
  return
    MUL_HI(t2, 10 * t - MUL_HI(t2, (uint16_t)(15 * F) - t * 6)) <<
    (noise_work_bits - noise_in_bits + 1);
#endif
}

#ifdef __ARM_NEON
# define LERP(t, a, b) (((a) >> lerp_loss) + vqdmulhq_s16(t, (b) - (a)))
#else
# define LERP(t, a, b) (((a) >> lerp_loss) + IMUL_HI(t, (b) - (a)))
#endif

#define SCALE(n) ((n) + (uint16_t)(1 << (noise_out_bits - 1)))

/* `a & b | ~a & c`, or `a ? b : c` */
#define PICK(a, b, c) ((((b) ^ (c)) & (a)) ^ (c))

static INLINE v_hi
grad (v_uhi hash, v_uhi x, v_uhi y, v_uhi z)
{
  v_uhi h = hash & 15;                  /* CONVERT LO 4 BITS OF HASH CODE */
  v_uhi u, v;                           /* INTO 12 GRADIENT DIRECTIONS. */
#if VSIZE == 1
  u = h<8 ? x : y;
  v = h<4 ? y : ((h & ~2) == 12 ? x : z);
  return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
#else
  /* GCC vector comparisons give 0 or -1 instead of 0 or 1. */
  v_uhi v1;
  u = PICK(h<8, x, y);
  v1 = PICK((h & ~2) == 12, x, z);
  v = PICK(h<4, y, v1);
  return (v_hi)((((h&1) != 0) ^ u) + (((h&2) != 0) ^ v));
#endif
}

/* Perlin's code used pre-computed random numbers. */
static INLINE v_uhi
noise_rand (v_uhi x)
{
  /* An 8-bit minimal perfect hash function. This isn't a good source of
     random numbers, but it's good enough here. Applied to a 16-bit value, the
     least significant bits are identical to the 8-bit version of this if the
     upper 8 bits are 0.
   */
  x ^= x >> 3;
  x ^= x << 1;
  return (x << 5) - x;
}

#define P(x) (noise_rand((x) & 0xff))

static INLINE v_uhi
noise (v_uhi x, v_uhi y, v_uhi z)
{
  const v_uhi one = broadcast(1 << noise_work_bits);
  v_uhi X, Y, Z, A, B, AA, AB, BA, BB;
  v_hi u, v, w;
  v_hi c0, c1, c2, c3, c4, c5, c6, c7;
  X = x >> noise_in_bits;			/* FIND UNIT CUBE THAT */
  Y = y >> noise_in_bits;			/* CONTAINS POINT. */
  Z = z >> noise_in_bits;
  x &= (uint16_t)((1 << noise_in_bits) - 1);	/* FIND RELATIVE X,Y,Z */
  y &= (uint16_t)((1 << noise_in_bits) - 1);	/* OF POINT IN CUBE. */
  z &= (uint16_t)((1 << noise_in_bits) - 1);
  u = (v_hi)fade(x);				/* COMPUTE FADE CURVES */
  v = (v_hi)fade(y);				/* FOR EACH OF X,Y,Z. */
  w = (v_hi)fade(z);

  A = noise_rand(X)+Y, AA = P(A)+Z, AB = P(A+1)+Z; /* HASH COORDINATES OF */
  B = P(X+1)+Y,        BA = P(B)+Z, BB = P(B+1)+Z; /* THE 8 CUBE CORNERS, */
  x <<= noise_work_bits - noise_in_bits;
  y <<= noise_work_bits - noise_in_bits;
  z <<= noise_work_bits - noise_in_bits;
  c0 = grad(P(AA  ), x,     y,     z     );
  c1 = grad(P(BA  ), x-one, y,     z     );
  c2 = grad(P(AB  ), x,     y-one, z     );
  c3 = grad(P(BB  ), x-one, y-one, z     );
  c4 = grad(P(AA+1), x,     y,     z-one );
  c5 = grad(P(BA+1), x-one, y,     z-one );
  c6 = grad(P(AB+1), x,     y-one, z-one );
  c7 = grad(P(BB+1), x-one, y-one, z-one );

  return
  (v_uhi)SCALE(LERP(w, LERP(v, LERP(u, c0, c1),  /* AND ADD BLENDED */
                               LERP(u, c2, c3)), /* RESULTS FROM  8 */
                       LERP(v, LERP(u, c4, c5),  /* CORNERS OF CUBE */
                               LERP(u, c6, c7))));
}


/* Fractal Brownian Motion
 */
static INLINE v_uhi
fbm (v_uhi x, v_uhi y, v_uhi z)
{    
  const float G = exp2f(-0.5);
  int octaves = 2;
  v_uhi f = broadcast(1);
#if __ARM_NEON
  const uint16_t iG = (uint16_t)(G * 0x10000);
  int16_t a = 0x7fff >> (noise_out_bits - noise_in_bits);
#else
  const v_uhi iG = broadcast((int16_t)(G * 0x10000));
  v_uhi a = broadcast((uint16_t)0xffff);
#endif
  v_uhi t = broadcast(0);
  int i;
  for (i = 0; i < octaves; i++)
    {
#if __ARM_NEON
      t += (v_uhi)vqdmulhq_n_s16((v_hi)noise (f*x, f*y, f*z), a);
      a = ((uint32_t)a * iG) >> 16;
#else
      t += MUL_HI(noise (f*x, f*y, f*z), a);
      a = MUL_HI(a, iG);
#endif
      f *= 2;
    }
  return t;
}


} // namespace

struct Marbling::State {
    unsigned w = 0, h = 0;
    int scale = 10, iterations = 5;
    v_uhi Z = broadcast(0);
    std::vector<uint8_t> pixels;
    concurrency::ParallelBatch batch;

    State()
        : batch(std::max(1u, std::thread::hardware_concurrency()))
    {
    }

    void draw()
    {
        batch.run([this](size_t id, size_t count) { run(id, count); });
        Z += (int16_t)(0.01 * (1 << noise_in_bits));
    }

    void run(unsigned thread_id, unsigned count)
    {
        unsigned x, y;
        float S = scale << noise_in_bits;

        for (y = thread_id; y < h; y += count)
          {
            v_uhi Y = broadcast((float) y / h * S);

#if VSIZE == 1
            uint32_t X = 0, Xd = 0x10000 / w * S;
#else
            v_uhi X, Xd = broadcast((float) VSIZE / w * S);
            for (x = 0; x != VSIZE; x++)
              VEC_INDEX(X, x) = (float) x / w * S;
#endif

            for (x = 0; x < w; x += VSIZE)
              {
                int i;
#if VSIZE == 1
                uint16_t X0 = X >> 16;
#else
                v_uhi X0 = X;
#endif
                v_uhi p = broadcast(0);
                for (i = 0; i < iterations; i++)
                  p = fbm (p+X0, p+Y, p+Z);

                for (i = 0; i != VSIZE; ++i)
                  pixels[(size_t)y * w + x + i] =
                    VEC_INDEX(p, i) & ((1 << noise_in_bits) - 1);
                X += Xd;
              }
          }
    }
};

Marbling::Marbling() : Art("Marbling"), m_state(std::make_unique<State>())
{
    usePlane();
    easel->pal.rescale(256);
}

Marbling::~Marbling() = default;

void Marbling::resize(int width, int height)
{
    default_resize(width, height);
    // Preserve the original SIMD alignment and sample-coordinate normalization.
    m_state->w = (((width + m_grid_size - 1) / m_grid_size + VSIZE - 1) & ~(VSIZE - 1));
    m_state->h = (height + m_grid_size - 1) / m_grid_size;
    m_state->pixels.resize((size_t)m_state->w * m_state->h);
    m_next_frame = 0;
}

bool Marbling::render(uint32_t*)
{
    if (!m_state->w || !m_state->h) return false;
    const double now = ImGui::GetTime();
    if (now < m_next_frame) return false;
    m_state->draw();
    std::array<uint32_t, 256> colors;
    const uint32_t color_count = easel->pal.get_color_count();
    for (unsigned i = 0; i < colors.size(); ++i) {
        // Quantize the full noise range into the selected palette size.
        const uint32_t color_index = ((i + m_color_offset) & 255) * color_count / 256;
        colors[i] = easel->pal.get_color(color_index);
    }
    for (int y = 0; y < easel->h; ++y) {
        const auto* row = m_state->pixels.data() + (size_t)(y / m_grid_size) * m_state->w;
        for (int x = 0; x < easel->w; ++x)
            drawdot(x, y, colors[row[x / m_grid_size]]);
    }
    m_next_frame = ImGui::GetTime() + m_delay / 1000000.0;
    return false;
}

bool Marbling::render_gui()
{
    if (ScrollableSliderInt("Magnification", &m_grid_size, 1, 20, "%d", 1))
        resize(easel->w, easel->h);
    ScrollableSliderInt("Scale", &m_state->scale, 1, 20, "%d", 1);
    ScrollableSliderInt("Complexity", &m_state->iterations, 1, 10, "%d", 1);
    ScrollableSliderInt("Frame delay (us)", &m_delay, 0, 100000, "%d", 1000);
    ImGui::Text("CPU workers: %zu", m_state->batch.size());
    return false;
}

void Marbling::shuffle()
{
    // The shared palette picker replaces XScreenSaver's random smooth colormap.
    m_color_offset = LRAND() % 256;
    m_next_frame = 0;
}

std::string Marbling::about() const
{
    return "Marbling by Jamie Zawinski and Dave Odell, 2021-2022, from XScreenSaver.\n\n"
           "Fixed-point Perlin noise is combined in two octaves of Fractal Brownian Motion. "
           "Each complexity iteration feeds the result back into all three coordinates, "
           "creating warped clouds and marble striations. Advancing the third coordinate "
           "animates the field. The original CPU SIMD and parallel row calculation are retained.\n\n"
           "Magnification sets the pixel block size; scale sets spatial density; complexity "
           "sets the number of domain-warping iterations. Frame delay is the pause after each "
           "computed frame. Use the palette picker to recolor, or Shuffle to rotate its colors.\n\n"
           "https://mrl.cs.nyu.edu/~perlin/noise/\n"
           "https://thebookofshaders.com/13/\n"
           "https://www.jwz.org/xscreensaver/";
}
