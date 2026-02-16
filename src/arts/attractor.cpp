
#include "imgui_elements.h"
#include "attractor.h"
#include "random.h"

void Attractor::init()
{
	clear();
	count = 0;
	attractor->reset();
}

bool Attractor::render(uint32_t *p)
{
	int x, y;
	double oldi, oldj;

	auto ftarget = easel->frame_vertex_target();
	auto vbmax = easel->vertex_buffer_maximum();
	for (int i=0; i < ftarget /*&& count < vbmax*/; ++i, ++count) {
		const auto [x, y] = attractor->get_point();
		drawdot(mul*x/easel->w, mul*y/easel->h);
	}

	return false;
}

bool Attractor::render_gui ()
{
	bool up = false;

	up |= ScrollableSliderDouble("mul", &mul, -256, 256, "%.4f", 2);
	up |= ScrollableSliderDouble("inc",  &attractor->iinc, -200, 200, "%.4f", 0.0001);
	up |= ScrollableSliderDouble("a",    &attractor->aa,   -20, 20,   "%.4f", 0.0001);
	up |= ScrollableSliderDouble("b",    &attractor->bb,   -20, 20,   "%.4f", 0.0001);
	up |= ScrollableSliderDouble("c",    &attractor->cc,   -20, 20,   "%.4f", 0.0001);
	up |= ScrollableSliderDouble("d",    &attractor->dd,   -20, 20,   "%.4f", 0.0001);
	up |= ScrollableSliderDouble("e, i", &attractor->ee,   -20, 20,   "%.4f", 0.0001);
	up |= ScrollableSliderDouble("f, j", &attractor->ff,   -20, 20,   "%.4f", 0.0001);

	ImGui::Text("count %ld", count);

	if (up) {
		init();
	}
	return false;
}

void Attractor::resize(int _w, int _h) {
	init();
}
