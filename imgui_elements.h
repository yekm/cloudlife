
bool BitField(const char* label, unsigned* bits, unsigned* hoverIndex);
bool ScrollableSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float scrollFactor);
bool ScrollableSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, float scrollFactor);
bool ScrollableSliderUInt(const char* label, unsigned* v, unsigned v_min, unsigned v_max, const char* format, float scrollFactor);
bool ScrollableSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float scrollFactor);
bool ScrollableSliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, float scrollFactor);

int cpu_load_text_now(char * text);
char *cpu_load_text();
void cpu_load_gui();
