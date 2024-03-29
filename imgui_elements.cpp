#include "imgui.h"
#include "imgui_internal.h" // ImGui::SetItemKeyOwner

// https://gist.github.com/JSandusky/af0e94011aee31f7b05ed2257d347637
bool BitField(const char* label, unsigned* bits, unsigned* hoverIndex)
{
    unsigned val = *bits;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    unsigned oldFlags = window->Flags;
    ImGuiContext* g = ImGui::GetCurrentContext();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, 0x0, true);
    const ImVec2 smallLabelSize = ImVec2(label_size.x * 0.5f, label_size.y * 0.5f);

    const float spacingUnit = 2.0f;

    bool anyPressed = false;
    ImVec2 currentPos = window->DC.CursorPos;
    for (unsigned i = 0; i < 32; ++i)
    {
        const void* lbl = (void*)(label + i);
        const ImGuiID localId = window->GetID(lbl);
        //if (i == 16)
        if (i== 10 || i == 20 || i == 30)
        {
            currentPos.x = window->DC.CursorPos.x;
            currentPos.y += smallLabelSize.y + style.FramePadding.y * 2 + spacingUnit /*little bit of space*/;
        }
        if (i == 5 || i == 15 || i == 25)
            currentPos.x += smallLabelSize.y;

        const ImRect check_bb(currentPos, { currentPos.x + smallLabelSize.y + style.FramePadding.y * 2, currentPos.y + smallLabelSize.y + style.FramePadding.y * 2 });

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(check_bb, localId, &hovered, &held, ImGuiButtonFlags_PressedOnClick);
        if (pressed)
            *bits ^= (1 << i);

        if (hovered && hoverIndex)
            *hoverIndex = i;

        ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImGui::GetColorU32((held && hovered) ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
        if (*bits & (1 << i))
        {
            const float check_sz = ImMin(check_bb.GetWidth(), check_bb.GetHeight());
            const float pad = ImMax(spacingUnit, (float)(int)(check_sz / 4.0f));
            window->DrawList->AddRectFilled(
            { check_bb.Min.x + pad, check_bb.Min.y + pad },
            { check_bb.Max.x - pad, check_bb.Max.y - pad }, ImGui::GetColorU32(ImGuiCol_CheckMark));
        }

        anyPressed |= pressed;
        currentPos.x = check_bb.Max.x + spacingUnit;
    }

    const ImRect matrix_bb(window->DC.CursorPos,
        { window->DC.CursorPos.x + (smallLabelSize.y + style.FramePadding.y * 2) * 16 /*# of checks in a row*/ + smallLabelSize.y /* space between sets of 5*/ + 9 * spacingUnit /*spacing between each check*/,
            window->DC.CursorPos.y + ((smallLabelSize.y + style.FramePadding.y * 2) * 5 /*# of rows*/ + spacingUnit /*spacing between rows*/) });

    ImGui::ItemSize(matrix_bb, style.FramePadding.y);

    ImRect total_bb = matrix_bb;

    if (label_size.x > 0)
        ImGui::SameLine(0, style.ItemInnerSpacing.x);

    const ImRect text_bb({ window->DC.CursorPos.x, window->DC.CursorPos.y + style.FramePadding.y }, { window->DC.CursorPos.x + label_size.x, window->DC.CursorPos.y + style.FramePadding.y + label_size.y });
    if (label_size.x > 0)
    {
        ImGui::ItemSize(ImVec2(text_bb.GetWidth(), matrix_bb.GetHeight()), style.FramePadding.y);
        total_bb = ImRect(ImMin(matrix_bb.Min, text_bb.Min), ImMax(matrix_bb.Max, text_bb.Max));
    }

    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    if (label_size.x > 0.0f)
        ImGui::RenderText(text_bb.GetTL(), label);

    window->Flags = oldFlags;
    return anyPressed;
}

template<typename T>
bool ScrollableSliderT(ImGuiDataType data_type, const char* label, T* v, T v_min, T v_max, const char* format, float scrollFactor)
{
    bool rv = ImGui::SliderScalar(label, data_type, v, &v_min, &v_max, format, ImGuiSliderFlags_None);
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    if (ImGui::IsItemHovered())
    {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel)
        {
            if (ImGui::IsItemActive())
            {
                ImGui::ClearActiveID();
            }
            else
            {
                *v += wheel * scrollFactor;
                if (*v < v_min) { *v = v_min; }
                else if (*v > v_max) { *v = v_max; }
                rv = true;
            }
        }
    }
    return rv;
}

bool ScrollableSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, float scrollFactor) {
    return ScrollableSliderT<int>(ImGuiDataType_S32, label, (int*)v, (int)v_min, (int)v_max, format, scrollFactor);
}

bool ScrollableSliderUInt(const char* label, unsigned* v, unsigned v_min, unsigned v_max, const char* format, float scrollFactor) {
    return ScrollableSliderT<unsigned>(ImGuiDataType_U32, label, v, v_min, v_max, format, scrollFactor);
}

bool ScrollableSliderDouble(const char* label, double* v, double v_min, double v_max, const char* format, float scrollFactor)
{
    return ScrollableSliderT<double>(ImGuiDataType_Double, label, v, v_min, v_max, format, scrollFactor);
}

bool ScrollableSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, float scrollFactor)
{
    return ScrollableSliderT<float>(ImGuiDataType_Float, label, v, v_min, v_max, format, scrollFactor);
}


#include <sys/resource.h>
#include "timer.h"

int cpu_load_text_now(char * text)
{
    static double up = 0, sp = 0;
    static common::Timer old_t;
    static struct rusage old_usage;
    static double outt = 0, ostt = 0;
    static double ru_maxrss = 0;

    common::Timer t;

    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    double utt = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    double stt = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;

    double dt = (t - old_t).seconds();
    up = 100.0 * (utt - outt) / dt;
    sp = 100.0 * (stt - ostt) / dt;

    //printf("%.3f %.3f %.3f %.3f %.3f %.3f %.3f\n", dt,
    //        up, sp, utt, stt, outt, ostt);

    old_t = t;
    outt = utt;
    ostt = stt;

    ru_maxrss = usage.ru_maxrss / 1024;

    return sprintf(text,
        "Usr + Sys = %.2f + %.2f = %.2f\nmaxrss %.2f MB",
        up, sp, up+sp, ru_maxrss);
}

char * cpu_load_text()
{
    static char text[1024*4];
    static int c = 0;
    char *t;

    if (c % 120 == 0) {
        cpu_load_text_now(text);
    }
    c++;

    return text;
}

void cpu_load_gui()
{
    ImGui::Text(cpu_load_text());
}

#if 0
using namespace ImGui;

// https://github.com/ocornut/imgui/issues/694#issuecomment-1004190040

static bool palettePanel(const char *title, const ImVec4* colors, int nColors, const ImVec2 &colorButtonSize, int *selectedPalIdx, int highlightPalIdx = -1, ImGuiWindowFlags flags = 0u) {
	if (ImGui::Begin(title, nullptr, flags)) {
		const ImVec2 &pos = ImGui::GetCursorScreenPos();
		bool colorHovered = false;
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImDrawListFlags backupFlags = drawList->Flags;
		drawList->Flags &= ~ImDrawListFlags_AntiAliasedLines;

		const ImU32 redColor = ImGui::GetColorU32(core::Color::Red);
		const ImU32 yellowColor = ImGui::GetColorU32(core::Color::Yellow);
		const ImU32 darkRedColor = ImGui::GetColorU32(core::Color::DarkRed);

		const float windowWidth = ImGui::GetWindowContentRegionMax().x;
		ImVec2 globalCursorPos = ImGui::GetCursorScreenPos();
		for (int palIdx = 0; palIdx < nColors; ++palIdx) {
			const float borderWidth = 1.0f;
			const ImVec2 v1(globalCursorPos.x + borderWidth, globalCursorPos.y + borderWidth);
			const ImVec2 v2(globalCursorPos.x + colorButtonSize.x, globalCursorPos.y + colorButtonSize.y);

			drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(colors[palIdx]));

			const core::String &id = core::string::format("##palitem-%i", palIdx);
			if (ImGui::InvisibleButton(id.c_str(), colorButtonSize)) {
				*selectedPalIdx = palIdx;
			}
#if 0
			const core::String &contextMenuId = core::string::format("Actions##context-palitem-%i", palIdx);
			if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
				ImGui::EndPopup();
			}
#endif
			if (!colorHovered && ImGui::IsItemHovered()) {
				colorHovered = true;
				drawList->AddRect(v1, v2, redColor);
			} else if (palIdx == highlightPalIdx) {
				drawList->AddRect(v1, v2, yellowColor);
			} else if (palIdx == *selectedPalIdx) {
				drawList->AddRect(v1, v2, darkRedColor);
			}
			globalCursorPos.x += colorButtonSize.x;
			if (globalCursorPos.x > windowWidth - colorButtonSize.x) {
				globalCursorPos.x = pos.x;
				globalCursorPos.y += colorButtonSize.y;
			}
			ImGui::SetCursorScreenPos(globalCursorPos);
		}
		const ImVec2 cursorPos(pos.x, globalCursorPos.y + colorButtonSize.y);
		ImGui::SetCursorScreenPos(cursorPos);

		// restore the draw list flags from above
		drawList->Flags = backupFlags;
		return true;
	}
	return false;
}
#endif
