/* acidwarp by Noah Spurrier https://noah.org/acidwarp/
 * Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>, Jun 2023
 */

#include <math.h>

#include "random.h"
#include "acidwarp.h"

#include "imgui_elements.h"

extern "C" {
#include "acidwarp/handy.h"
#include "acidwarp/acidwarp.h"
#include "acidwarp/rolnfade.h"
#include "acidwarp/palinit.h"
}

int RES = 3;
int XMax = 1023, YMax = 767;

int GO = TRUE;
int SKIP = FALSE;
int NP = FALSE; /* flag indicates new palette */
int LOCK = FALSE; /* flag indicates don't change to next image */

std::vector<UCHAR> buf_graf;

UCHAR MainPalArray [256 * 3];
UCHAR TargetPalArray [256 * 3];


#include <stdlib.h>

uint32_t pal_rgb(int i) {
  if (i > 256)
    printf("%d ", i);
  return 0xff000000 |
         (MainPalArray[i*3+0]*4) << 0 |
         (MainPalArray[i*3+1]*4) << 8 |
         (MainPalArray[i*3+2]*4) << 16;
}

bool AcidWarp::render(uint32_t *p) {

  ++frame;

  switch(acid_state) {
  case IMAGE:
    generate_image(image_func, buf_graf.data(), XMax/2, YMax/2, XMax, YMax, 255);
    //pal_num = xoshiro256plus() % NUM_PALETTE_TYPES;
    initPalArray(TargetPalArray, pal_num);
    acid_state = FADE_IN;
    //break;
  case FADE_IN:
    rolNFadeMainPalAryToTargNLodDAC(MainPalArray, TargetPalArray);
    if (frame > frame_max)
      frame = 0, acid_state = ROTATE;
    break;
  case ROTATE:
    rollMainPalArrayAndLoadDACRegs(MainPalArray);
    if (frame > frame_max)
      frame = 0, acid_state = FADE_OUT;
    break;
  case FADE_OUT:
    // FIXME: something wrong with rolFade*, draw a pallete in imgui, see whats going on
    if (fade_dir)
      rolNFadeBlkMainPalArrayNLoadDAC(MainPalArray);
    else
      rolNFadeWhtMainPalArrayNLoadDAC(MainPalArray);
    if (frame > frame_max)
      frame = 0, acid_state = IMAGE;
    break;
  }

  for (int x = 0; x < XMax; ++x)
    for(int y = 0; y < YMax; ++y)
    {
      auto c = pal_rgb(buf_graf.at(x + y*XMax));
      //printf("%x ", c);
      drawdot(p, x, y, c);
    }

  return true;
}

bool AcidWarp::render_gui() {
  bool up = false, re = false;

  re |= ScrollableSliderInt("image func", &image_func, 0, NUM_IMAGE_FUNCTIONS+1, "%d", 1);
  re |= ScrollableSliderInt("pal num", &pal_num, 0, NUM_PALETTE_TYPES, "%d", 1);
  ScrollableSliderInt("frames each state", &frame_max, 0, 60*300, "%d", 60);
  //up |= ScrollableSliderInt("resolution", &RES, 0, 3, "%d", 0);
  ImGui::Checkbox("fade_dir", &fade_dir);

  if (re) {
    frame = 0;
    acid_state = IMAGE;
    initPalArray(TargetPalArray, pal_num);
  }

  pal.RenderGui();

  ImGui::Text("state:%d frame:%d", acid_state, frame);


  return up;
}

void AcidWarp::resize(int _w, int _h) {
  XMax = _w - 1;
  YMax = _h - 1;
  //tex_w = XMax+1;
  //tex_h = YMax+1;

  initPalArray(MainPalArray, pal_num);
  initPalArray(TargetPalArray, pal_num);

  buf_graf.resize(tex_w*tex_h);
  fill0(buf_graf);

  //default_resize(width, height);
  //pal.rescale(ncolors);


    frame = 0;
    acid_state = IMAGE;

}
