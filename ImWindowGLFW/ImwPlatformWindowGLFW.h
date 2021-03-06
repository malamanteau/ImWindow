
#ifndef __IM_PLATFORM_WINDOW_GLFW_H__
#define __IM_PLATFORM_WINDOW_GLFW_H__

#include <mutex>
#include <string>

#include "../ImWindow/ImwConfig.h"
#include "../ImWindow/ImwPlatformWindow.h"


#ifdef USE_GLES3
	
	#define GL_GLEXT_PROTOTYPES
	// OpenGL ES includes
	#include <GLES3/gl3.h>
	//#include <GLES3/gl3ext.h>

	// EGL includes
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <EGL/eglplatform.h>
	#define GLFW_INCLUDE_NONE

	//#include "../imgui.h"
	//#include "imgui_impl_glfw.h"

	// GLFW
	#include <GLFW/glfw3.h>

	#ifdef _WIN32
		#undef APIENTRY
		#define GLFW_EXPOSE_NATIVE_WIN32
		#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
	#endif
#else
	#include "GLFW/glfw3.h"
#endif





IMGUI_API ImTextureID ImGui_Impl_CreateImageRGBA8888(uint8_t const * pixels, int32_t width, int32_t height);
IMGUI_API void        ImGui_Impl_DeleteImage(ImTextureID img);

namespace ImWindow
{
	class ImwPlatformWindowGLFW : ImwPlatformWindow
	{
		friend class ImwWindowManagerGLFW;

	public:
		static std::string GLFWProgramWindowName;
											ImwPlatformWindowGLFW(EPlatformWindowType eType, bool bCreateState);
		virtual								~ImwPlatformWindowGLFW();

		virtual bool						Init(ImwPlatformWindow* pMain);

		virtual ImVec2						GetPosition() const;
		virtual ImVec2						GetSize() const;
		virtual bool						IsWindowMaximized() const;
		virtual bool						IsWindowMinimized() const;

		virtual void						Show(bool bShow);
		virtual void						SetSize(int iWidth, int iHeight);
		virtual void						SetPosition(int iX, int iY);
		virtual void						SetWindowMaximized(bool bMaximized);
		virtual void						SetWindowMinimized();
		virtual void						SetTitle(const ImwChar* pTtile);

	protected:
		virtual void						PreUpdate();
		virtual void						RenderDrawLists(ImDrawData* draw_data);
		virtual void                        UpdateTime();

		static void							OnClose(GLFWwindow* pWindow);
		static void							OnFocus(GLFWwindow* pWindow, int iFocus);
		static void							OnSize(GLFWwindow* pWindow, int iWidth, int iHeight);
		static void							OnMouseButton(GLFWwindow* pWindow, int iButton, int iAction, int iMods);
		static void							OnMouseMove(GLFWwindow* pWindow, double fPosX, double fPosY);
		static void							OnMouseWheel(GLFWwindow* pWindow, double fOffsetX, double fOffsetY);
		static void							OnKey(GLFWwindow* pWindow, int iKey, int iScanCode, int iAction, int iMods);
		static void							OnChar(GLFWwindow* pWindow, unsigned int iCodepoint);
		static void                         OnEnter(GLFWwindow* pWindow, int entered);

		GLFWwindow*							m_pWindow;
		GLFWcursor*							m_pCursorArrow;
		GLFWcursor*							m_pCursorCrosshair;
		GLFWcursor*							m_pCursorHand;
		GLFWcursor*							m_pCursorIBeam;
		GLFWcursor*							m_pCursorHResize;
		GLFWcursor*							m_pCursorVResize;
		int									m_iLastMods;
		ImTextureID							m_iTextureID;

		double                              m_Time = 0.0f;
	};
}

#endif // __IM_PLATFORM_WINDOW_GLFW_H__
