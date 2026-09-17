/* Lyap - calculate and display Lyapunov exponents */

/* Written by Ron Record (rr@sco) 03 Sep 1991 */

/* The idea here is to calculate the Lyapunov exponent for a periodically
 * forced logistic map (later i added several other nonlinear maps of the unit
 * interval). In order to turn the 1-dimensional parameter space of the
 * logistic map into a 2-dimensional parameter space, select two parameter
 * values ('a' and 'b') then alternate the iterations of the logistic map using
 * first 'a' then 'b' as the parameter. This program accepts an argument to
 * specify a forcing function, so instead of just alternating 'a' and 'b', you
 * can use 'a' as the parameter for say 6 iterations, then 'b' for 6 iterations
 * and so on. An interesting forcing function to look at is abbabaab (the
 * Morse-Thue sequence, an aperiodic self-similar, self-generating sequence).
 * Anyway, step through all the values of 'a' and 'b' in the ranges you want,
 * calculating the Lyapunov exponent for each pair of values. The exponent
 * is calculated by iterating out a ways (specified by the variable "settle")
 * then on subsequent iterations calculating an average of the logarithm of
 * the absolute value of the derivative at that point. Points in parameter
 * space with a negative Lyapunov exponent are colored one way (using the
 * value of the exponent to index into a color map) while points with a
 * non-negative exponent are colored differently.
 *
 * The algorithm was taken from the September 1991 Scientific American article
 * by A. K. Dewdney who gives credit to Mario Markus of the Max Planck
 * Institute for its creation. Additional information and ideas were gleaned
 * from the discussion on alt.fractals involving Stephen Hall, Ed Kubaitis,
 * Dave Platt and Baback Moghaddam. Assistance with colormaps and spinning
 * color wheels and X was gleaned from Hiram Clawson. Rubber banding code was
 * adapted from an existing Mandelbrot program written by Stacey Campbell.
 */

#define LYAP_PATCHLEVEL 4
#define LYAP_VERSION "#(@) lyap 2.3 2/20/92"

#include "xlyap.hpp"

#include "easelplane.h"
#include "random.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

// The imported mathematical routines and preset table retain their original formatting.
// X11 drawing/resources are replaced by instance state, EaselPlane and ImGui controls.
namespace xlyap {

#define ABS(a)  (((a)<0) ? (0-(a)) : (a) )
#define MAXCOLOR 256
#define MAXINDEX 64
#define NUMMAPS 5
#define NBUILTINS 23 // The original table includes case 22 as well.
#define TRUE 1
#define FALSE 0

typedef double (*PFD)(double,double);

struct state {
    PFD map = nullptr, deriv = nullptr;
    int dwell = 50, settle = 50;
    int width = 0, height = 0;
    struct { int x = 0, y = 0; } point;
    int run = 1, mapindex = 0;
    int aflag = 0, bflag = 0, wflag = 0, hflag = 0;
    int maxindex = 8, force = 0, Rflag = 0;
    int forcing[MAXINDEX] = {};
    double min_a = 2.0, min_b = 2.0, a_range = 2.0, b_range = 2.0;
    double max_a = 4.0, max_b = 4.0, a_inc = 0.0, b_inc = 0.0;
    double a = 0.0, b = 0.0, start_x = 0.65, lyapunov = 0.0;
    double minlyap = 1.0, maxexp = 1.0, minexp = -1.0, prob = 0.5;
    int maxcolor = MAXCOLOR, startcolor = 0, mincolindex = 33;
    int numcolors = 200, numfreecols = 167, lowrange = 33;
    int color_offset = 0, negative = 1, useprod = 1;
    int sendpoint_index = 0;
    int preset = -1;
    bool randomize = true, paused = false;
    float linger = 5.0f;
    double completed_at = -1.0;
    char forcing_text[MAXINDEX + 1] = "abbabaab";
    char deterministic_text[MAXINDEX + 1] = "abbabaab";
    int threads = 1;
    std::vector<double> exponents;
    std::vector<unsigned char> ready;
    size_t completed = 0, recolor = 0;
    std::array<uint32_t, MAXCOLOR> colors = {};
};

// Workers own this small orbit state; UI settings and cached image data stay on the render thread.
struct computation {
    PFD map = nullptr, deriv = nullptr;
    int dwell = 0, settle = 0, width = 0, maxindex = 0, Rflag = 0, useprod = 1;
    int forcing[MAXINDEX] = {};
    double min_a = 0.0, min_b = 0.0, a_inc = 0.0, b_inc = 0.0;
    double a = 0.0, b = 0.0, start_x = 0.0, lyapunov = 0.0, prob = 0.5;
    std::mt19937 random;
};

struct job {
    computation parameters;
    int height = 0, threads = 1;
    std::atomic<bool> cancelled{false}, paused{false};
    std::mutex pause_mutex;
    std::condition_variable pause_cv;

    bool checkpoint()
    {
        if (cancelled.load(std::memory_order_relaxed))
            return false;
        if (paused.load(std::memory_order_relaxed)) {
            std::unique_lock<std::mutex> lock(pause_mutex);
            pause_cv.wait(lock, [&] { return cancelled.load() || !paused.load(); });
        }
        return !cancelled.load(std::memory_order_relaxed);
    }

    void set_paused(bool value)
    {
        // Synchronize the predicate change with wait to avoid a missed notification.
        { std::lock_guard<std::mutex> lock(pause_mutex); paused = value; }
        pause_cv.notify_all();
    }

    void cancel()
    {
        { std::lock_guard<std::mutex> lock(pause_mutex); cancelled = true; }
        pause_cv.notify_all();
    }
};

static void setforcing(struct computation *st);

static const double pmins[NUMMAPS] = { 2.0, 0.0, 0.0, 0.0, 0.0 };
static const double pmaxs[NUMMAPS] = { 4.0, 1.0, 6.75, 6.75, 16.0 };
static const double amins[NUMMAPS] = { 2.0, 0.0, 0.0, 0.0, 0.0 };
static const double aranges[NUMMAPS] = { 2.0, 1.0, 6.75, 6.75, 16.0 };
static const double bmins[NUMMAPS] = { 2.0, 0.0, 0.0, 0.0, 0.0 };
static const double branges[NUMMAPS] = { 2.0, 1.0, 6.75, 6.75, 16.0 };

/****************************************************************************/

/* callback function declarations
 */

static double logistic(double,double);
static double circle(double,double);
static double leftlog(double,double);
static double rightlog(double,double);
static double doublelog(double,double);
static double dlogistic(double,double);
static double dcircle(double,double);
static double dleftlog(double,double);
static double drightlog(double,double);
static double ddoublelog(double,double);

static const PFD Maps[NUMMAPS] = { logistic, circle, leftlog, rightlog, 
                                   doublelog };
static const PFD Derivs[NUMMAPS] = { dlogistic, dcircle, dleftlog, 
                                     drightlog, ddoublelog };


/* complyap() is the guts of the program. This is where the Lyapunov exponent
 * is calculated. For each iteration (past some large number of iterations)
 * calculate the logarithm of the absolute value of the derivative at that
 * point. Then average them over some large number of iterations. Some small
 * speed up is achieved by utilizing the fact that log(a*b) = log(a) + log(b).
 */
static int
complyap(struct computation *st, int pixel_x, int pixel_y, struct job& work)
{
  int i, bindex;
  double total, prod, x, dx, r;

  if (!work.checkpoint())
    return FALSE;
  // The original increments a before evaluating. The caller copies the last
  // sample into the right edge instead of evaluating that column again.
  st->a = st->min_a + std::min(pixel_x + 1, st->width - 1) * st->a_inc;
  st->b = st->min_b + pixel_y * st->b_inc;
  prod = 1.0;
  total = 0.0;
  bindex = 0;
  x = st->start_x;
  r = (st->forcing[bindex]) ? st->b : st->a;
#ifdef MAPS
  findex = 0;
  map = Maps[st->Forcing[findex]];
#endif
  for (i=0;i<st->settle;i++) {     /* Here's where we let the thing */
    if ((i & 63) == 0 && !work.checkpoint())
      return FALSE;
    x = st->map (x, r);  /* "settle down". There is usually */
    if (++bindex >= st->maxindex) { /* some initial "noise" in the */
      bindex = 0;    /* iterations. How can we optimize */
      if (st->Rflag)      /* the value of settle ??? */
        setforcing(st);
    }
    r = (st->forcing[bindex]) ? st->b : st->a;
#ifdef MAPS
    if (++findex >= funcmaxindex)
      findex = 0;
    map = Maps[st->Forcing[findex]];
#endif
  }
#ifdef MAPS
  deriv = Derivs[st->Forcing[findex]];
#endif
  if (st->useprod) {      /* using log(a*b) */
    for (i=0;i<st->dwell;i++) {
      if ((i & 63) == 0 && !work.checkpoint())
        return FALSE;
      x = st->map (x, r);
      dx = st->deriv (x, r); /* ABS is a macro, so don't be fancy */
      dx = ABS(dx);
      if (dx == 0.0) /* log(0) is nasty so break out. */
        {
          i++;
          break;
        }
      prod *= dx;
      /* we need to prevent overflow and underflow */
      if ((prod > 1.0e12) || (prod < 1.0e-12)) {
        total += log(prod);
        prod = 1.0;
      }
      if (++bindex >= st->maxindex) {
        bindex = 0;
        if (st->Rflag)
          setforcing(st);
      }
      r = (st->forcing[bindex]) ? st->b : st->a;
#ifdef MAPS
      if (++findex >= funcmaxindex)
        findex = 0;
      map = Maps[st->Forcing[findex]];
      deriv = Derivs[st->Forcing[findex]];
#endif
    }
    total += log(prod);
    st->lyapunov = (total * M_LOG2E) / (double)i;   
  }
  else {        /* use log(a) + log(b) */
    for (i=0;i<st->dwell;i++) {
      if ((i & 63) == 0 && !work.checkpoint())
        return FALSE;
      x = st->map (x, r);
      dx = st->deriv (x, r); /* ABS is a macro, so don't be fancy */
      dx = ABS(dx);
      if (x == 0.0)  /* log(0) check */
        {
          i++;
          break;
        }
      total += log(dx);
      if (++bindex >= st->maxindex) {
        bindex = 0;
        if (st->Rflag)
          setforcing(st);
      }
      r = (st->forcing[bindex]) ? st->b : st->a;
#ifdef MAPS
      if (++findex >= funcmaxindex)
        findex = 0;
      map = Maps[st->Forcing[findex]];
      deriv = Derivs[st->Forcing[findex]];
#endif
    }
    st->lyapunov = (total * M_LOG2E) / (double)i;
  }

  return TRUE;
}

static double
logistic(double x, double r)        /* the familiar logistic map */
{
  return(r * x * (1.0 - x));
}

static double
dlogistic(double x, double r)       /* the derivative of logistic map */
{
  return(r - (2.0 * r * x));
}

static double
circle(double x, double r)        /* sin() hump or sorta like the circle map */
{
  return(r * sin(M_PI * x));
}

static double
dcircle(double x, double r)       /* derivative of the "sin() hump" */
{
  return(r * M_PI * cos(M_PI * x));
}

static double
leftlog(double x, double r)       /* left skewed logistic */
{
  double d;

  d = 1.0 - x;
  return(r * x * d * d);
}

static double
dleftlog(double x, double r)    /* derivative of the left skewed logistic */
{
  return(r * (1.0 - (4.0 * x) + (3.0 * x * x)));
}

static double
rightlog(double x, double r)    /* right skewed logistic */
{
  return(r * x * x * (1.0 - x));
}

static double
drightlog(double x, double r)    /* derivative of the right skewed logistic */
{
  return(r * ((2.0 * x) - (3.0 * x * x)));
}

static double
doublelog(double x, double r)    /* double logistic */
{
  double d;

  d = 1.0 - x;
  return(r * x * x * d * d);
}

static double
ddoublelog(double x, double r)   /* derivative of the double logistic */
{
  double d;

  d = x * x;
  return(r * ((2.0 * x) - (6.0 * d) + (4.0 * x * d)));
}

/* Here's where we index into a color map. After the Lyapunov exponent is
 * calculated, it is used to determine what color to use for that point.  I
 * suppose there are a lot of ways to do this. I used the following : if it's
 * non-negative then there's a reserved area at the lower range of the color
 * map that i index into. The ratio of some "minimum exponent value" and the
 * calculated value is used as a ratio of how high to index into this reserved
 * range. Usually these colors are dark red (see init_color).  If the exponent
 * is negative, the same ratio (expo/minlyap) is used to index into the
 * remaining portion of the colormap (which is usually some light shades of
 * color or a rainbow wheel). The coloring scheme can actually make a great
 * deal of difference in the quality of the picture. Different colormaps bring
 * out different details of the dynamics while different indexing algorithms
 * also greatly effect what details are seen. Play around with this.
 */
static int
color_index(struct state *st, double expo)
{
  double tmpexpo;

  // Infinite/escaped orbits must never be converted directly to integer indices.
  if (!std::isfinite(expo))
    return std::isnan(expo) ? st->startcolor :
           ((expo < 0) == (st->negative != 0) ? st->mincolindex : st->startcolor);
  tmpexpo = (st->negative) ? expo : -1.0 * expo;
  double index;
  int range, base;
  if (tmpexpo > 0) {
    index = tmpexpo*st->lowrange/st->maxexp;
    range = st->lowrange;
    base = st->startcolor;
  }
  else {
    index = tmpexpo*st->numfreecols/st->minexp;
    range = st->numfreecols;
    base = st->mincolindex;
  }
  if (!std::isfinite(index))
    return base;
  // fmod avoids overflowing an integer for a very large finite exponent.
  st->sendpoint_index = static_cast<int>(std::fmod(index, range)) + base;
  st->sendpoint_index = (st->sendpoint_index + st->color_offset) % st->numcolors;
  if (st->sendpoint_index < 0)
    st->sendpoint_index += st->numcolors;
  return st->sendpoint_index;
}

static void
setforcing(struct computation *st)
{
  int i;
  for (i=0;i<MAXINDEX;i++)
    st->forcing[i] = (double(st->random()) / std::mt19937::max() > st->prob) ? 0 : 1;
}

// A fixed pool is reused across generations. Jobs/results never reference Art or OpenGL.
class worker_pool {
public:
    struct result {
        std::shared_ptr<job> generation;
        size_t first = 0;
        std::vector<double> exponents;
    };

    static int maximum_threads()
    {
        const unsigned cores = std::thread::hardware_concurrency();
        return static_cast<int>(std::min(32u, cores > 1 ? cores - 1 : 1u));
    }

    explicit worker_pool(int count)
    {
        try {
            for (int i = 0; i < count; ++i)
                m_threads.emplace_back([this, i] { run(i); });
        } catch (...) {
            // A partially constructed vector of joinable threads cannot unwind.
            { std::lock_guard<std::mutex> lock(m_mutex); m_stopping = true; }
            m_cv.notify_all();
            for (auto& thread : m_threads)
                thread.join();
            throw;
        }
    }

    ~worker_pool()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
            if (m_job)
                m_job->cancel();
        }
        m_cv.notify_all();
        for (auto& thread : m_threads)
            thread.join();
    }

    void start(std::shared_ptr<job> work)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_job)
                m_job->cancel();
            m_job = std::move(work);
            m_next = 0;
            m_results.clear();
        }
        m_cv.notify_all();
    }

    void set_paused(bool paused)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_job)
            m_job->set_paused(paused);
        m_cv.notify_all();
    }

    bool take(result& tile)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_results.empty())
            return false;
        tile = std::move(m_results.front());
        m_results.pop_front();
        m_cv.notify_all();
        return true;
    }

private:
    void run(int worker_index)
    {
        std::shared_ptr<job> previous;
        computation orbit;
        for (;;) {
            std::shared_ptr<job> work;
            size_t first, count;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] {
                    return m_stopping || (m_job && !m_job->paused.load() &&
                        worker_index < m_job->threads &&
                        (!m_job->parameters.Rflag || worker_index == 0) &&
                        m_next < size_t(m_job->parameters.width) * m_job->height);
                });
                if (m_stopping)
                    return;
                work = m_job;
                first = m_next;
                // Row-aligned spans make right-edge reuse local. Avoid a one-pixel
                // final span, which would need the preceding span's exponent.
                const size_t remaining = work->parameters.width - first % work->parameters.width;
                count = remaining == 129 ? 129 : std::min(size_t(128), remaining);
                m_next += count;
            }
            if (previous != work) {
                orbit = work->parameters;
                previous = work;
            }
            result tile{work, first, {}};
            tile.exponents.reserve(count);
            for (size_t offset = 0; offset < count; ++offset) {
                const size_t index = first + offset;
                const int x = static_cast<int>(index % orbit.width);
                const int y = static_cast<int>(index / orbit.width);
                if (!work->checkpoint())
                    break;
                if (!(orbit.width > 1 && x == orbit.width - 1) &&
                    !complyap(&orbit, x, y, *work))
                    break;
                tile.exponents.push_back(orbit.lyapunov);
            }
            if (tile.exponents.size() != count)
                continue;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] {
                    return m_stopping || m_job != work ||
                        m_results.size() < size_t(std::max(4, work->threads * 2));
                });
                if (m_stopping)
                    return;
                if (m_job == work)
                    m_results.push_back(std::move(tile));
            }
        }
    }

    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopping = false;
    size_t m_next = 0;
    std::shared_ptr<job> m_job;
    std::deque<result> m_results;
    std::vector<std::thread> m_threads;
};

static void
do_preset (struct state *st, int builtin)
{
  const char *ff = 0;
  switch (builtin) {
  case 0:
    st->min_a = 3.75; st->aflag++;
    st->min_b = 3.299999; st->bflag++;
    st->a_range = 0.05; st->wflag++;
    st->b_range = 0.05; st->hflag++;
    st->dwell = 200;
    st->settle = 100;
    ff = "abaabbaaabbb";
    break;

  case 1:
    st->min_a = 3.8; st->aflag++;
    st->min_b = 3.2; st->bflag++;
    st->b_range = .05; st->hflag++;
    st->a_range = .05; st->wflag++;
    ff = "bbbbbaaaaa";
    break;

  case 2:
    st->min_a =  3.4; st->aflag++;
    st->min_b =  3.04; st->bflag++;
    st->a_range =  .5; st->wflag++;
    st->b_range =  .5; st->hflag++;
    ff = "abbbbbbbbb";
    st->settle = 500;
    st->dwell = 1000;
    break;

  case 3:
    st->min_a = 3.5; st->aflag++;
    st->min_b = 3.0; st->bflag++;
    st->a_range = 0.2; st->wflag++;
    st->b_range = 0.2; st->hflag++;
    st->dwell = 600;
    st->settle = 300;
    ff = "aaabbbab";
    break;

  case 4:
    st->min_a = 3.55667; st->aflag++;
    st->min_b = 3.2; st->bflag++;
    st->b_range = .05; st->hflag++;
    st->a_range = .05; st->wflag++;
    ff = "bbbbbaaaaa";
    break;

  case 5:
    st->min_a = 3.79; st->aflag++;
    st->min_b = 3.22; st->bflag++;
    st->b_range = .02999; st->hflag++;
    st->a_range = .02999; st->wflag++;
    ff = "bbbbbaaaaa";
    break;

  case 6:
    st->min_a = 3.7999; st->aflag++;
    st->min_b = 3.299999; st->bflag++;
    st->a_range = 0.2; st->wflag++;
    st->b_range = 0.2; st->hflag++;
    st->dwell = 300;
    st->settle = 150;
    ff = "abaabbaaabbb";
    break;

  case 7:
    st->min_a = 3.89; st->aflag++;
    st->min_b = 3.22; st->bflag++;
    st->b_range = .028; st->hflag++;
    st->a_range = .02999; st->wflag++;
    ff = "bbbbbaaaaa";
    st->settle = 600;
    st->dwell = 1000;
    break;

  case 8:
    st->min_a = 3.2; st->aflag++;
    st->min_b = 3.7; st->bflag++;
    st->a_range = 0.05; st->wflag++;
    st->b_range = .005; st->hflag++;
    ff = "abbbbaa";
    break;

  case 9:
    ff = "aaaaaabbbbbb";
    st->mapindex = 1;
    st->dwell =  400;
    st->settle =  200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 10:
    ff = "aaaaaabbbbbb";
    st->mapindex = 1;
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 11:
    st->mapindex = 1;
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 12:
    ff = "abbb";
    st->mapindex = 1;
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 13:
    ff = "abbabaab";
    st->mapindex = 1;
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 14:
    ff = "abbabaab";
    st->dwell =  800;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    /* ####  -x 0.05 */
    st->min_a = 3.91; st->aflag++;
    st->a_range =  0.0899999999; st->wflag++;
    st->min_b =  3.28; st->bflag++;
    st->b_range =  0.35; st->hflag++;
    break;

  case 15:
    ff = "aaaaaabbbbbb";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 16:
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 17:
    ff = "abbb";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 18:
    ff = "abbabaab";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 19:
    st->mapindex = 2;
    ff = "aaaaaabbbbbb";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 20:
    st->mapindex = 2;
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 21:
    st->mapindex = 2;
    ff = "abbb";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  case 22:
    st->mapindex = 2;
    ff = "abbabaab";
    st->dwell =  400;
    st->settle = 200;
    st->minlyap = st->maxexp = ABS(-0.85);
    st->minexp = -1.0 * st->minlyap;
    break;

  default: 
    abort();
    break;
  }

  if (ff) {
    const char *ch;
    int bindex = 0;
    st->maxindex = strlen(ff);
    if (st->maxindex > MAXINDEX)
      abort();
    ch = ff;
    st->force++;
    while (bindex < st->maxindex) {
      if (*ch == 'a')
        st->forcing[bindex++] = 0;
      else if (*ch == 'b')
        st->forcing[bindex++] = 1;
      else
        abort();
      ch++;
    }
  }
}


} // namespace xlyap

XLyap::XLyap()
    : Art("XLyap — Lyapunov exponents"), m_state(std::make_unique<xlyap::state>())
{
    usePlane();
    m_state->threads = xlyap::worker_pool::maximum_threads();
    apply_preset(static_cast<int>(LRAND() % NBUILTINS));
}

XLyap::~XLyap() = default;

std::string XLyap::about() const
{
    return "XLyap calculates Lyapunov exponents for periodically forced maps of the unit interval. "
           "Ron Record wrote Lyap on 3 September 1991; this port retains the original map and derivative "
           "formulas, exponent iteration loops and preset table from XScreenSaver's xlyap.c.\n\n"
           "The algorithm came from A. K. Dewdney's September 1991 Scientific American article, crediting "
           "Mario Markus at the Max Planck Institute. Each pixel chooses parameters a and b. A sequence "
           "such as abbabaab selects the parameter at each iteration. The orbit first settles, then the "
           "program averages the logarithm of the absolute derivative in base two. Negative exponents "
           "indicate stable dynamics; positive exponents indicate sensitivity to initial conditions.\n\n"
           "Choose any of the original 23 presets or edit the map, forcing, ranges and iteration counts. "
           "The five maps are logistic, sine hump, left and right skewed logistic, and double logistic. "
           "Random forcing selects b with the requested probability. Palette controls use Cloudlife's "
           "shared palettes in place of X11 colormaps. Color changes reuse the calculated exponents. "
           "Background CPU workers fill image tiles while the UI stays responsive; the CPU workers "
           "control selects the parallelism. Random forcing runs sequentially on one worker with a "
           "private random generator, so its random stream differs from earlier versions. "
           "Completed images regenerate after the linger interval when "
           "automatic regeneration is enabled. Shuffle chooses a new preset.\n\n"
           "https://www.jwz.org/xscreensaver/\n"
           "https://en.wikipedia.org/wiki/Lyapunov_fractal";
}

void XLyap::apply_preset(int preset)
{
    auto* st = m_state.get();
    // Reset the original resource defaults so presets never inherit a previous preset's settings.
    const bool randomize = st->randomize;
    const float linger = st->linger;
    const bool paused = st->paused;
    const int width = st->width, height = st->height, threads = st->threads;
    *st = xlyap::state{};
    st->randomize = randomize;
    st->linger = linger;
    st->paused = paused;
    st->threads = threads;
    st->width = width;
    st->height = height;
    for (int i = 0; i < st->maxindex; ++i)
        st->forcing[i] = st->forcing_text[i] == 'b';
    if (preset >= 0)
        xlyap::do_preset(st, preset);
    st->preset = preset;
    if (!st->aflag) st->min_a = xlyap::amins[st->mapindex];
    if (!st->bflag) st->min_b = xlyap::bmins[st->mapindex];
    if (!st->wflag) st->a_range = xlyap::aranges[st->mapindex];
    if (!st->hflag) st->b_range = xlyap::branges[st->mapindex];
    for (int i = 0; i < st->maxindex; ++i)
        st->forcing_text[i] = st->forcing[i] ? 'b' : 'a';
    st->forcing_text[st->maxindex] = '\0';
    std::strcpy(st->deterministic_text, st->forcing_text);
    restart();
}

void XLyap::restart()
{
    auto* st = m_state.get();
    st->map = xlyap::Maps[st->mapindex];
    st->deriv = xlyap::Derivs[st->mapindex];
    st->max_a = st->min_a + st->a_range;
    st->max_b = st->min_b + st->b_range;
    st->a_inc = st->width > 0 ? st->a_range / st->width : 0.0;
    st->b_inc = st->height > 0 ? st->b_range / st->height : 0.0;
    st->point.x = st->point.y = 0;
    st->completed = st->recolor = 0;
    st->completed_at = -1.0;
    st->run = st->width > 0 && st->height > 0;
    st->exponents.assign(static_cast<size_t>(st->width) * st->height, 0.0);
    st->ready.assign(st->exponents.size(), 0);
    st->recolor = st->exponents.size();
    if (st->run) {
        clear();
        if (!m_workers)
            m_workers = std::make_unique<xlyap::worker_pool>(xlyap::worker_pool::maximum_threads());
        auto work = std::make_shared<xlyap::job>();
        auto& parameters = work->parameters;
        parameters.map = st->map;
        parameters.deriv = st->deriv;
        parameters.dwell = st->dwell;
        parameters.settle = st->settle;
        parameters.width = st->width;
        parameters.maxindex = st->maxindex;
        parameters.Rflag = st->Rflag;
        parameters.useprod = st->useprod;
        std::copy(std::begin(st->forcing), std::end(st->forcing), std::begin(parameters.forcing));
        parameters.min_a = st->min_a;
        parameters.min_b = st->min_b;
        parameters.a_inc = st->a_inc;
        parameters.b_inc = st->b_inc;
        parameters.start_x = st->start_x;
        parameters.prob = st->prob;
        if (st->Rflag) {
            parameters.random.seed(static_cast<unsigned>(LRAND()));
            xlyap::setforcing(&parameters);
        }
        work->height = st->height;
        work->threads = st->Rflag ? 1 : st->threads;
        work->paused = st->paused;
        m_workers->start(std::move(work));
    } else if (m_workers) {
        m_workers->start(nullptr);
    }
}

void XLyap::resize(int width, int height)
{
    m_state->width = std::max(0, width);
    m_state->height = std::max(0, height);
    restart();
}

void XLyap::shuffle()
{
    apply_preset(static_cast<int>(LRAND() % NBUILTINS));
}

bool XLyap::render_gui()
{
    auto* st = m_state.get();
    bool restart_needed = false, recolor_needed = false;
    if (ImGui::BeginCombo("Preset", st->preset < 0 ? "Custom / defaults" :
                         ("Original " + std::to_string(st->preset)).c_str())) {
        if (ImGui::Selectable("Defaults", st->preset < 0))
            apply_preset(-1);
        for (int i = 0; i < NBUILTINS; ++i) {
            const std::string label = "Original " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), st->preset == i))
                apply_preset(i);
        }
        ImGui::EndCombo();
    }
    if (ImGui::Combo("Map", &st->mapindex,
                     "Logistic\0Sine hump\0Left skewed logistic\0Right skewed logistic\0Double logistic\0")) {
        st->min_a = xlyap::amins[st->mapindex];
        st->min_b = xlyap::bmins[st->mapindex];
        st->a_range = xlyap::aranges[st->mapindex];
        st->b_range = xlyap::branges[st->mapindex];
        restart_needed = true;
    }
    ImGui::InputText("Forcing (a/b)", st->forcing_text, sizeof(st->forcing_text));
    bool valid_forcing = st->forcing_text[0] != '\0';
    for (const char* ch = st->forcing_text; *ch; ++ch)
        valid_forcing &= *ch == 'a' || *ch == 'b';
    if (valid_forcing) {
        std::strcpy(st->deterministic_text, st->forcing_text);
        const int length = static_cast<int>(std::strlen(st->forcing_text));
        bool different = length != st->maxindex;
        for (int i = 0; i < length; ++i)
            different |= st->forcing[i] != (st->forcing_text[i] == 'b');
        if (different && !st->Rflag) {
            st->maxindex = length;
            for (int i = 0; i < length; ++i)
                st->forcing[i] = st->forcing_text[i] == 'b';
            restart_needed = true;
        }
    } else {
        ImGui::TextUnformatted("Enter 1–64 letters, using only a and b.");
    }
    bool random_force = st->Rflag != 0;
    if (ImGui::Checkbox("Random forcing", &random_force)) {
        st->Rflag = random_force;
        if (!random_force) {
            if (!valid_forcing)
                std::strcpy(st->forcing_text, st->deterministic_text);
            st->maxindex = static_cast<int>(std::strlen(st->forcing_text));
            for (int i = 0; i < st->maxindex; ++i)
                st->forcing[i] = st->forcing_text[i] == 'b';
        } else if (random_force) {
            st->maxindex = MAXINDEX;
        }
        restart_needed = true;
    }
    if (random_force) {
        float prob = static_cast<float>(st->prob);
        if (ImGui::SliderFloat("Probability of b", &prob, 0.0f, 1.0f)) {
            st->prob = prob;
            restart_needed = true;
        }
    }
    restart_needed |= ImGui::SliderInt("Dwell", &st->dwell, 1, 10000);
    restart_needed |= ImGui::SliderInt("Settle", &st->settle, 0, 10000);
    bool use_log = !st->useprod;
    if (ImGui::Checkbox("Sum logarithms", &use_log)) {
        st->useprod = !use_log;
        restart_needed = true;
    }
    restart_needed |= ImGui::InputDouble("Minimum a", &st->min_a, 0.001, 0.1, "%.9g");
    restart_needed |= ImGui::InputDouble("Minimum b", &st->min_b, 0.001, 0.1, "%.9g");
    restart_needed |= ImGui::InputDouble("Range a", &st->a_range, 0.001, 0.1, "%.9g");
    restart_needed |= ImGui::InputDouble("Range b", &st->b_range, 0.001, 0.1, "%.9g");
    restart_needed |= ImGui::InputDouble("Start x", &st->start_x, 0.001, 0.01, "%.9g");
    st->dwell = std::clamp(st->dwell, 1, 10000);
    st->settle = std::clamp(st->settle, 0, 10000);
    st->a_range = std::max(1.0e-12, st->a_range);
    st->b_range = std::max(1.0e-12, st->b_range);
    if (st->min_a < xlyap::pmins[st->mapindex] ||
        st->min_a + st->a_range > xlyap::pmaxs[st->mapindex] ||
        st->min_b < xlyap::pmins[st->mapindex] ||
        st->min_b + st->b_range > xlyap::pmaxs[st->mapindex])
        ImGui::TextUnformatted("Parameter range extends outside this map's unit-interval domain.");
    recolor_needed |= ImGui::InputDouble("Color exponent", &st->minlyap, 0.05, 0.1, "%.4g");
    st->minlyap = std::max(1.0e-6, std::abs(st->minlyap));
    st->maxexp = st->minlyap;
    st->minexp = -st->minlyap;
    recolor_needed |= ImGui::SliderInt("Colors", &st->numcolors, 3, MAXCOLOR);
    recolor_needed |= ImGui::SliderInt("Minimum color index", &st->mincolindex, 1, st->numcolors - 1);
    st->mincolindex = std::clamp(st->mincolindex, 1, st->numcolors - 1);
    st->lowrange = st->mincolindex - st->startcolor;
    st->numfreecols = st->numcolors - st->mincolindex;
    recolor_needed |= ImGui::SliderInt("Color offset", &st->color_offset, 0, st->numcolors - 1);
    bool negative = st->negative != 0;
    if (ImGui::Checkbox("Negative exponent palette", &negative)) {
        st->negative = negative;
        recolor_needed = true;
    }
    restart_needed |= ImGui::SliderInt("CPU workers", &st->threads, 1,
                                       xlyap::worker_pool::maximum_threads());
    if (st->Rflag)
        ImGui::TextUnformatted("Random forcing uses one worker to preserve sequential forcing.");
    if (ImGui::Checkbox("Pause", &st->paused) && m_workers)
        m_workers->set_paused(st->paused);
    ImGui::Checkbox("Automatic regeneration", &st->randomize);
    ImGui::SliderFloat("Linger (seconds)", &st->linger, 1.0f, 120.0f);
    if (ImGui::Button("Recalculate"))
        restart_needed = true;
    if (restart_needed) {
        st->preset = -1;
        restart();
    } else if (recolor_needed) {
        st->recolor = st->completed == 0 ? st->exponents.size() : 0;
    }
    const float progress = st->exponents.empty() ? 0.0f :
                          static_cast<float>(st->completed) / st->exponents.size();
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
    return false;
}

bool XLyap::render(uint32_t*)
{
    auto* st = m_state.get();
    if (st->width <= 0 || st->height <= 0)
        return false;
    // Shared palette edits recolor cached exponents without recalculating the map.
    std::array<uint32_t, MAXCOLOR> colors;
    for (int i = 0; i < MAXCOLOR; ++i)
        colors[i] = easel->pal.get_colorf(static_cast<float>(std::min(i, st->numcolors - 1)) /
                                           (st->numcolors - 1));
    if (colors != st->colors) {
        st->colors = colors;
        st->recolor = st->completed == 0 ? st->exponents.size() : 0;
    }
    const auto started = std::chrono::steady_clock::now();
    const auto deadline = started + std::chrono::milliseconds(8);
    const auto recolor_deadline = started + std::chrono::milliseconds(4);
    while (st->recolor < st->exponents.size() && std::chrono::steady_clock::now() < recolor_deadline) {
        const size_t index = st->recolor++;
        if (st->ready[index])
            drawdot(static_cast<int>(index % st->width), static_cast<int>(index / st->width),
                    st->colors[xlyap::color_index(st, st->exponents[index])]);
    }
    // Tiles may finish out of order. Each arriving exponent gets the current
    // palette immediately, independently of the cached-image recolor cursor.
    xlyap::worker_pool::result tile;
    while (m_workers && std::chrono::steady_clock::now() < deadline && m_workers->take(tile)) {
        for (size_t offset = 0; offset < tile.exponents.size(); ++offset) {
            const size_t index = tile.first + offset;
            st->exponents[index] = tile.exponents[offset];
            st->ready[index] = 1;
            drawdot(static_cast<int>(index % st->width), static_cast<int>(index / st->width),
                    st->colors[xlyap::color_index(st, st->exponents[index])]);
        }
        st->completed += tile.exponents.size();
    }
    if (st->run && st->completed == st->exponents.size()) {
        st->run = 0;
        st->completed_at = ImGui::GetTime();
    }
    if (!st->paused && !st->run && st->randomize && st->completed_at >= 0.0 &&
        ImGui::GetTime() - st->completed_at >= st->linger)
        shuffle();
    return false;
}
