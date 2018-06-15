
#include "ImwWindowManagerGLFW.h"
#include "../sample.h"

#include "../../imgui/imgui_internal.h"

ImFontAtlas * Globals::SharedFontAtlas = nullptr;

int main()
{
	ImFontAtlas fa;

	Globals::SharedFontAtlas = &fa;
	auto * context = ImGui::CreateContext(Globals::SharedFontAtlas);

	PreInitSample();

	ImwWindowManagerGLFW oMgr;

	oMgr.Init();

	InitSample();

	while (oMgr.Run(false) && oMgr.Run(true))
		; // Sleep(16);

	ImGui::DestroyContext(context);

	return 0;
}
