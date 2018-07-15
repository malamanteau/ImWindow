#include "ImwWindowManagerGLFW.h"
#include "ImwPlatformWindowGLFW.h"

#include "../../HandyCpp/Handy.hpp"

#ifdef IS_WINDOWS
	#include "windows.h"
#endif

using namespace ImWindow;

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

ImwWindowManagerGLFW::ImwWindowManagerGLFW()
{
	glfwSetErrorCallback(error_callback);
	fprintf(stderr, "Calling glfwInit()...\r\n"); fflush(stderr);
	glfwInit();
	fprintf(stderr, "glfwInit() returned.\r\n"); fflush(stderr);
}

ImwWindowManagerGLFW::~ImwWindowManagerGLFW()
{
	Destroy();
	glfwTerminate();
}

ImwPlatformWindow* ImwWindowManagerGLFW::CreatePlatformWindow(EPlatformWindowType eType, ImwPlatformWindow* pParent)
{
	IM_ASSERT(m_pCurrentPlatformWindow == NULL);
	ImwPlatformWindowGLFW* pWindow = new ImwPlatformWindowGLFW(eType, CanCreateMultipleWindow());
	if (pWindow->Init(pParent))
	{
		return (ImwPlatformWindow*)pWindow;
	}
	else
	{
		delete pWindow;
		return NULL;
	}
}

ImVec2 ImwWindowManagerGLFW::GetCursorPos()
{
	//TODO Make ImwWindowManagerGLFW::GetCursorPos multiplatform
#ifdef IS_WINDOWS
	POINT oPoint;
	::GetCursorPos(&oPoint);
	return ImVec2(oPoint.x, oPoint.y);
#else
	return ImVec2();
#endif
}

bool ImwWindowManagerGLFW::IsLeftClickDown()
{
	//TODO Make ImwWindowManagerGLFW::IsLeftClickDown multiplatform
#ifdef IS_WINDOWS
	return GetAsyncKeyState(VK_LBUTTON);
#else
	return false;
#endif
}
