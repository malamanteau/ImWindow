
//System Includes
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "../../HandyCpp/Handy.hpp"
#include "../../HandyCpp/HandyExtended.hpp"

#include "ImwPlatformWindowGLFW.h"

#include "../../imgui/misc/freetype/imgui_freetype.h"

#ifdef LOADGLFWICON
	#include "../../HandyCpp/Extended/stb_image.h"
#endif


using namespace ImWindow;

static GLFWwindow * offscreen_context = nullptr;

#define GL_CLAMP_TO_EDGE 0x812F

static std::mutex ImGui_Impl_Mutex;

static Handy::ThreadPool * g_pool = nullptr;

ImTextureID ImGui_Impl_CreateImageRGBA8888(uint8_t const * pixels, int32_t width, int32_t height)
{
	if (!g_pool)
	{
		if (!offscreen_context)
			return 0;

		g_pool = new Handy::ThreadPool(1);

		g_pool->AddJob([] 
		{
			glfwMakeContextCurrent(offscreen_context);
		});
	}

	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);
	
	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	GLuint textureID = 0;

	g_pool->AddJob([&textureID,width,height,pixels] 
	{
		glEnable(GL_TEXTURE_2D);
		//glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		//PFNGLGENERATEMIPMAPPROC(GL_TEXTURE_2D);

		if (!textureID)
		{
			bool hi = false;
		}

		glFinish();
		glFlush();
	});

	g_pool->WaitAll();
	//if (textureID == 0)
	//	std::cerr << "Zero texture ID" << std::endl;

	return (ImTextureID)textureID;
}

void ImGui_Impl_DeleteImage(ImTextureID img)
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

	g_pool->AddJob([img] 
	{
		GLuint id = (GLuint)((uintptr_t)img);

		if (id)
			glDeleteTextures(1, &id);
	});

	//g_pool->WaitAll();
}

ImwPlatformWindowGLFW::ImwPlatformWindowGLFW(EPlatformWindowType eType, bool bCreateState)
	: ImwPlatformWindow(eType, bCreateState)
	, m_pWindow( NULL )
	, m_pCursorArrow( NULL )
	, m_pCursorCrosshair( NULL )
	, m_pCursorHand( NULL )
	, m_pCursorIBeam( NULL )
	, m_pCursorHResize( NULL )
	, m_pCursorVResize( NULL )
	, m_iLastMods( 0 )
	, m_iTextureID( 0 )
{
}

ImwPlatformWindowGLFW::~ImwPlatformWindowGLFW()
{
	if (m_pCursorArrow != NULL)
		glfwDestroyCursor(m_pCursorArrow);
	if (m_pCursorCrosshair != NULL)
		glfwDestroyCursor(m_pCursorCrosshair);
	if (m_pCursorHand != NULL)
		glfwDestroyCursor(m_pCursorHand);
	if (m_pCursorIBeam != NULL)
		glfwDestroyCursor(m_pCursorIBeam);
	if (m_pCursorHResize != NULL)
		glfwDestroyCursor(m_pCursorHResize);
	if (m_pCursorVResize != NULL)
		glfwDestroyCursor(m_pCursorVResize);


	if (m_eType == E_PLATFORM_WINDOW_TYPE_MAIN)
	{
		if (m_iTextureID != 0)
		{
			glDeleteTextures(1, &m_iTextureID);
			m_iTextureID = 0;
		}
	}

	if (m_pWindow != NULL)
	{
		glfwDestroyWindow(m_pWindow);
	}
}

bool ImwPlatformWindowGLFW::Init(ImwPlatformWindow* pMain)
{
	if (!offscreen_context)
	{
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		offscreen_context = glfwCreateWindow(640, 480, "", NULL, offscreen_context);
	}

	ImwPlatformWindowGLFW* pMainGLFW = ((ImwPlatformWindowGLFW*)pMain);

	GLFWwindow* pMainWindow = NULL;
	if (pMain != NULL)
	{
		pMainWindow = ((ImwPlatformWindowGLFW*)pMain)->m_pWindow;
	}

	if (GetType() == E_PLATFORM_WINDOW_TYPE_MAIN ||
		GetType() == E_PLATFORM_WINDOW_TYPE_SECONDARY)
	{
		glfwWindowHint(GLFW_DECORATED, 1);
	}
	else
	{
		glfwWindowHint(GLFW_DECORATED, 0);
	}

	glfwWindowHint(GLFW_VISIBLE, 0);

	//#ifdef _WIN32
	//	glfwWindowHint(GLFW_DOUBLEBUFFER, 0);
	//#else
		//glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	//#endif
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	m_pWindow = glfwCreateWindow(1024, 768, "Main Window GLFW", NULL, pMainWindow ? pMainWindow : offscreen_context);
	
	#ifdef LOADGLFWICON
	{
		int width = 0, height = 0, bpp = 0;
		unsigned char * rgba = nullptr;

		try
		{
			std::filesystem::path thispath(Handy::Platform::Paths::ThisExecutableFile());
			
			std::ifstream myImg;

			std::string asStr = (thispath.parent_path() / "Internal" / "glfwicon.png").string();

			myImg.open((thispath.parent_path() / "Internal" / "glfwicon.png").c_str(), std::ios::in | std::ios::binary);

			if (myImg.good())
			{
				myImg.seekg(0, std::ios::end);
				std::streampos length(myImg.tellg());
				std::vector<char> vec;
				if (length)
				{
					myImg.seekg(0, std::ios::beg);
					vec.resize(static_cast<std::size_t>(length));
					myImg.read(&vec.front(), static_cast<std::size_t>(length));
				}

				if ((rgba = stbi_load_from_memory((const stbi_uc *)&vec[0], (int)length, &width, &height, &bpp, 4)))
				{
					GLFWimage glfwImg = { width, height, rgba };
					glfwSetWindowIcon(m_pWindow, 1, &glfwImg);
				}
			}

			myImg.close();
		}
		catch (...)
		{
			std::cerr << "Could not decode icon image from file: " << "internal/glfwicon.ico" << std::endl;
		}

		if (rgba)
			stbi_image_free(rgba);
	}
	#endif

	glfwSetWindowUserPointer(m_pWindow, this);

	glfwSetWindowCloseCallback(m_pWindow, &ImwPlatformWindowGLFW::OnClose);
	glfwSetWindowFocusCallback(m_pWindow, &ImwPlatformWindowGLFW::OnFocus);
	glfwSetWindowSizeCallback(m_pWindow, &ImwPlatformWindowGLFW::OnSize);
	glfwSetMouseButtonCallback(m_pWindow, &ImwPlatformWindowGLFW::OnMouseButton);
	glfwSetCursorPosCallback(m_pWindow, &ImwPlatformWindowGLFW::OnMouseMove);
	glfwSetScrollCallback(m_pWindow, &ImwPlatformWindowGLFW::OnMouseWheel);
	glfwSetKeyCallback(m_pWindow, &ImwPlatformWindowGLFW::OnKey);
	glfwSetCharCallback(m_pWindow, &ImwPlatformWindowGLFW::OnChar);
	glfwSetCursorEnterCallback(m_pWindow, &ImwPlatformWindowGLFW::OnEnter);
	
	//TODO alpha on dragging preview window

	glfwMakeContextCurrent(m_pWindow);

	//if (glfwExtensionSupported("WGL_EXT_swap_control_tear") || glfwExtensionSupported("GLX_EXT_swap_control_tear"))
	//	glfwSwapInterval(-1);
	//else
	//	glfwSwapInterval(1);

	SetContext(false);
	ImGuiIO& io = ImGui::GetIO();
	
	if (pMainGLFW != NULL)
	{
		//Copy texture reference
		m_iTextureID = pMainGLFW->m_iTextureID;
	}
	else
	{
		uint8_t* pPixels;
		int32_t iWidth;
		int32_t iHeight;
		auto * defFnt = io.Fonts->AddFontDefault();
		//auto * prettyFnt = io.Fonts->AddFontFromFileTTF("../../../imgui/misc/fonts/Roboto-Regular.ttf", 16);

		//#ifdef USE_FREETYPE
		unsigned int flags = 0;// ImGuiFreeType::LightHinting;
		ImGuiFreeType::BuildFontAtlas( io.Fonts, flags );
		//#endif

		//io.Fonts->GetTexDataAsAlpha8(&pPixels, &iWidth, &iHeight);

		io.Fonts->GetTexDataAsRGBA32(&pPixels, &iWidth, &iHeight);

		// Upload texture to graphics system
		glEnable(GL_TEXTURE_2D);
		m_iTextureID = 0;
		glGenTextures(1, &m_iTextureID);
		glBindTexture(GL_TEXTURE_2D, m_iTextureID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iWidth, iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);

		// Store our identifier
		io.Fonts->TexID = (void *)(intptr_t)m_iTextureID;

		ImGui::SetCurrentFont(defFnt);
		//ImGui::SetCurrentFont(prettyFnt);
	}
	
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	//io.ImeWindowHandle = m_pWindow->GetHandle();

	RestoreContext(false);

	m_pCursorArrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
	m_pCursorCrosshair = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
	m_pCursorHand = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	m_pCursorIBeam = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
	m_pCursorHResize = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
	m_pCursorVResize = glfwCreateStandardCursor	(GLFW_VRESIZE_CURSOR);

	return true;
}

ImVec2 ImwPlatformWindowGLFW::GetPosition() const
{
	int iX, iY;
	glfwGetWindowPos(m_pWindow, &iX, &iY);
	return ImVec2(float(iX), float(iY));
}

ImVec2 ImwPlatformWindowGLFW::GetSize() const
{
	int iX, iY;
	glfwGetWindowSize(m_pWindow, &iX, &iY);
	return ImVec2(float(iX), float(iY));
}

bool ImwPlatformWindowGLFW::IsWindowMaximized() const
{
	return glfwGetWindowAttrib(m_pWindow, GLFW_MAXIMIZED) == 1;
}

bool ImwPlatformWindowGLFW::IsWindowMinimized() const
{
	return glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED) == 1;
}

void ImwPlatformWindowGLFW::Show(bool bShow)
{
	if (bShow)
		glfwShowWindow(m_pWindow);
	else
		glfwHideWindow(m_pWindow);
}

void ImwPlatformWindowGLFW::SetSize(int iWidth, int iHeight)
{
	glfwSetWindowSize(m_pWindow, iWidth, iHeight);
}

void ImwPlatformWindowGLFW::SetPosition(int iX, int iY)
{
	glfwSetWindowPos(m_pWindow, iX, iY);
}

void ImwPlatformWindowGLFW::SetWindowMaximized(bool bMaximized)
{
	if (bMaximized)
		glfwMaximizeWindow(m_pWindow);
	else
		glfwRestoreWindow(m_pWindow);
}

void ImwPlatformWindowGLFW::SetWindowMinimized()
{
	glfwIconifyWindow(m_pWindow);
}

void ImwPlatformWindowGLFW::SetTitle(const ImwChar* pTitle)
{
	glfwSetWindowTitle(m_pWindow, pTitle);
}

void ImwPlatformWindowGLFW::PreUpdate()
{
	glfwMakeContextCurrent(m_pWindow);
	glfwPollEvents();

	ImGuiIO& oIO = m_pContext->IO;
	oIO.KeyCtrl = 0 != (m_iLastMods & GLFW_MOD_CONTROL);
	oIO.KeyShift = 0 != (m_iLastMods & GLFW_MOD_SHIFT);
	oIO.KeyAlt = 0 != (m_iLastMods & GLFW_MOD_ALT);
	oIO.KeySuper = 0 != (m_iLastMods & GLFW_MOD_SUPER);

	if (oIO.MouseDrawCursor)
	{
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else if (oIO.MousePos.x != -FLT_MAX && oIO.MousePos.y != -FLT_MAX)
	{
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		switch (m_pContext->MouseCursor)
		{
		case ImGuiMouseCursor_Arrow:
			glfwSetCursor(m_pWindow, m_pCursorArrow);
			break;
		case ImGuiMouseCursor_TextInput:         // When hovering over InputText, etc.
			glfwSetCursor(m_pWindow, m_pCursorIBeam);
			break;
		case ImGuiMouseCursor_ResizeAll:              // Unused
			glfwSetCursor(m_pWindow, m_pCursorHand);
			break;
		case ImGuiMouseCursor_ResizeNS:          // Unused
			glfwSetCursor(m_pWindow, m_pCursorVResize);
			break;
		case ImGuiMouseCursor_ResizeEW:          // When hovering over a column
			glfwSetCursor(m_pWindow, m_pCursorHResize);
			break;
		case ImGuiMouseCursor_ResizeNESW:        // Unused
			glfwSetCursor(m_pWindow, m_pCursorCrosshair);
			break;
		case ImGuiMouseCursor_ResizeNWSE:        // When hovering over the bottom-right corner of a window
			glfwSetCursor(m_pWindow, m_pCursorCrosshair);
			break;
		}
	}
}

void ImwPlatformWindowGLFW::OnClose(GLFWwindow* pWindow)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->ImwPlatformWindow::OnClose();
	glfwSetWindowShouldClose(pWindow, GLFW_FALSE);
}

void ImwPlatformWindowGLFW::OnFocus(GLFWwindow* pWindow, int iFocus)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->ImwPlatformWindow::OnFocus(iFocus != 0);
}

void ImwPlatformWindowGLFW::OnSize(GLFWwindow* pWindow, int iWidth, int iHeight)
{
	glfwMakeContextCurrent(pWindow);
	glViewport(0, 0, iWidth, iHeight);
}

void ImwPlatformWindowGLFW::OnMouseButton(GLFWwindow* pWindow, int iButton, int iAction, int iMods)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->m_iLastMods = iMods;
	if (iAction == GLFW_PRESS)
		pPlatformWindow->m_pContext->IO.MouseDown[iButton] = true;
	else if (iAction == GLFW_RELEASE)
		pPlatformWindow->m_pContext->IO.MouseDown[iButton] = false;
}

void ImwPlatformWindowGLFW::OnMouseMove(GLFWwindow* pWindow, double fPosX, double fPosY)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);

	pPlatformWindow->m_pContext->IO.MousePos = ImVec2((float)fPosX, (float)fPosY);
}

void ImwPlatformWindowGLFW::OnMouseWheel(GLFWwindow* pWindow, double fOffsetX, double fOffsetY)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->m_pContext->IO.MouseWheel += (float)fOffsetY;
}

void ImwPlatformWindowGLFW::OnKey(GLFWwindow* pWindow, int iKey, int iScanCode, int iAction, int iMods)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->m_iLastMods = iMods;
	if (iAction == GLFW_PRESS)
		pPlatformWindow->m_pContext->IO.KeysDown[iKey] = true;
	else if (iAction == GLFW_RELEASE)
		pPlatformWindow->m_pContext->IO.KeysDown[iKey] = false;
}


void ImwPlatformWindowGLFW::OnChar(GLFWwindow* pWindow, unsigned int iChar)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);
	pPlatformWindow->m_pContext->IO.AddInputCharacter((ImwChar)iChar);
}

void ImwPlatformWindowGLFW::OnEnter(GLFWwindow* pWindow, int entered)
{
	ImwPlatformWindowGLFW* pPlatformWindow = (ImwPlatformWindowGLFW*)glfwGetWindowUserPointer(pWindow);

	if (entered != 0) // if entered
	{

	}
	else
	{
		pPlatformWindow->m_pContext->IO.MousePos = ImVec2(-1_f, -1_f);
	}
}


void ImwPlatformWindowGLFW::UpdateTime()
{
	auto & io = ImGui::GetIO();

	double current_time = glfwGetTime(); // Setup time step

	io.DeltaTime = m_Time > 0.0 ? (float)(current_time - m_Time) : (float)(1.0f / 60.0f);
	m_Time = current_time;
}

void ImwPlatformWindowGLFW::RenderDrawLists(ImDrawData* pDrawData)
{
	if (m_pWindow != NULL)
	{
		glfwMakeContextCurrent(m_pWindow);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
		ImGuiIO& io = ImGui::GetIO();
		int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
		int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
		if (fb_width == 0 || fb_height == 0)
			return;
		pDrawData->ScaleClipRects(io.DisplayFramebufferScale);

		// We are using the OpenGL fixed pipeline to make the example code simpler to read!
		// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
		GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnable(GL_TEXTURE_2D);
		//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context

		// Setup viewport, orthographic projection matrix
		glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0f, io.DisplaySize.x, io.DisplaySize.y, 0.0f, -1.0f, +1.0f);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		// Render command lists
	#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
		for (int n = 0; n < pDrawData->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = pDrawData->CmdLists[n];
			const unsigned char* vtx_buffer = (const unsigned char*)&cmd_list->VtxBuffer.front();
			const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();
			glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, pos)));
			glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, uv)));
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer + OFFSETOF(ImDrawVert, col)));

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback)
				{
					pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
					glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
					glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
				}
				idx_buffer += pcmd->ElemCount;
			}
		}
	#undef OFFSETOF

		// Restore modified state
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		glBindTexture(GL_TEXTURE_2D, (GLuint)last_texture);
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glPopAttrib();

		glfwSwapBuffers(m_pWindow);
	}
}
