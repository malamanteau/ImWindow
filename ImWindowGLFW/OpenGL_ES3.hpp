#pragma once

#ifndef IS_INTELLISENSE

#pragma comment(lib, ANGLEBINPATH "lib\\libGLESv2.lib")
#pragma comment(lib, ANGLEBINPATH "lib\\libEGL.lib")

#endif

// OpenGL Data
static int          g_ShaderHandle           = 0;
static int          g_VertHandle             = 0;
static int          g_FragHandle             = 0;
static int          g_AttribLocationTex      = 0;
static int          g_AttribLocationProjMtx  = 0;
static int          g_AttribLocationPosition = 0;
static int          g_AttribLocationUV       = 0;
static int          g_AttribLocationColor    = 0;
static unsigned int g_VboHandle              = 0;
static unsigned int g_ElementsHandle         = 0;

void GLVersionWindowHint()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
}

void ImGui_Impl_CreateDeviceObjects()
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

	glfwMakeContextCurrent(offscreen_context);

	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	// Create shaders
	const GLchar *vertex_shader_with_version =
		"#version 300 es\n"
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* fragment_shader_with_version =
		"#version 300 es\n"
		"precision highp float;\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
		//"	Out_Color = vec4(1.0 - Out_Color.rgb, Out_Color.a);\n"
		"}\n";

	g_ShaderHandle = glCreateProgram();
	g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
	g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(g_VertHandle, 1, &vertex_shader_with_version, NULL);
	glShaderSource(g_FragHandle, 1, &fragment_shader_with_version, NULL);

	glCompileShader(g_VertHandle);
	int success;
	glGetShaderiv(g_VertHandle, GL_COMPILE_STATUS, &success);
	if (success == 0)
	{
		int infologLength = 0;
		char infoLog[1024];

		glGetShaderInfoLog(g_VertHandle, 1024, &infologLength, infoLog);

		std::cerr << "Failed to create vertex shader object." << std::endl;

		if (infologLength > 0)
			fprintf(stderr, "%s\n", infoLog);

		throw std::runtime_error("GLSL compile failure.");
	}


	glCompileShader(g_FragHandle);
	if (success == 0)
	{
		int infologLength = 0;
		char infoLog[1024];

		glGetShaderInfoLog(g_FragHandle, 1024, &infologLength, infoLog);

		std::cerr << "Failed to create fragment shader object." << std::endl;

		if (infologLength > 0)
			fprintf(stderr, "%s\n", infoLog);

		throw std::runtime_error("GLSL compile failure.");
	}

	glAttachShader(g_ShaderHandle, g_VertHandle);
	glAttachShader(g_ShaderHandle, g_FragHandle);

	glLinkProgram(g_ShaderHandle);
	glGetProgramiv(g_ShaderHandle, GL_LINK_STATUS, &success);
	if (success == 0)
	{
		int infologLength = 0;
		char infoLog[1024];

		glGetProgramInfoLog(g_ShaderHandle, 1024, &infologLength, infoLog);

		if (infologLength > 0)
			std::cerr << infoLog << std::endl;

		throw std::runtime_error("GLSL program link failure.");
	}

	g_AttribLocationTex      = glGetUniformLocation(g_ShaderHandle, "Texture");
	g_AttribLocationProjMtx  = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
	g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
	g_AttribLocationUV       = glGetAttribLocation(g_ShaderHandle, "UV");
	g_AttribLocationColor    = glGetAttribLocation(g_ShaderHandle, "Color");

	glGenBuffers(1, &g_VboHandle);
	glGenBuffers(1, &g_ElementsHandle);

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);
}

void ImGui_Impl_DestroyDeviceObjects()
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

	glfwMakeContextCurrent(offscreen_context);

	if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
	if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
	g_VboHandle = g_ElementsHandle = 0;

	if (g_ShaderHandle && g_VertHandle) glDetachShader(g_ShaderHandle, g_VertHandle);
	if (g_VertHandle) glDeleteShader(g_VertHandle);
	g_VertHandle = 0;

	if (g_ShaderHandle && g_FragHandle) glDetachShader(g_ShaderHandle, g_FragHandle);
	if (g_FragHandle) glDeleteShader(g_FragHandle);
	g_FragHandle = 0;

	if (g_ShaderHandle) glDeleteProgram(g_ShaderHandle);
	g_ShaderHandle = 0;

}

ImTextureID ImGui_Impl_CreateImageRGBA8888(uint8_t const * pixels, int32_t width, int32_t height)
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

	if (!offscreen_context)
		return 0;

	glfwMakeContextCurrent(offscreen_context);

	GLuint textureID = 0;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	if (!textureID)
	{
		fprintf(stderr, "Zero-value texture ID.\r\n");
	}

	glFinish();
	
	if (textureID == 0)
		std::cerr << "Zero texture ID" << std::endl;

	return (ImTextureID)textureID;
}

void ImGui_Impl_DeleteImage(ImTextureID img)
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

	glfwMakeContextCurrent(offscreen_context);

	GLuint id = (GLuint)((uintptr_t)img);

	if (id)
		glDeleteTextures(1, &id);
}

void ImwPlatformWindowGLFW::RenderDrawLists(ImDrawData* draw_data)
{
	std::lock_guard<std::mutex> lock(ImGui_Impl_Mutex);

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
		draw_data->ScaleClipRects(io.DisplayFramebufferScale);

		// Backup GL state
		//GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
		glActiveTexture(GL_TEXTURE0);
		//GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
		//GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
		//GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
		//GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
		//GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
		////GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
		//GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
		//GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
		//GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
		//GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
		//GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
		//GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
		//GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
		//GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
		//GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
		//GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
		//GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
		//GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

		// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Setup viewport, orthographic projection matrix
		// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
		glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		const float ortho_projection[4][4] =
		{
			{ 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
			{ 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
			{ 0.0f,         0.0f,        -1.0f,   0.0f },
			{ (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
		};
		glUseProgram(g_ShaderHandle);
		glUniform1i(g_AttribLocationTex, 0);
		glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
		/*if (glBindSampler) */glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.

													// Recreate the VAO every time 
													// (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts, and we don't track creation/deletion of windows so we don't have an obvious key to use to cache them.)
		GLuint vao_handle = 0;
		glGenVertexArrays(1, &vao_handle);
		glBindVertexArray(vao_handle);
		glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
		glEnableVertexAttribArray(g_AttribLocationPosition);
		glEnableVertexAttribArray(g_AttribLocationUV);
		glEnableVertexAttribArray(g_AttribLocationColor);
		glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
		glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
		glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

		// Draw
		ImVec2 pos = draw_data->DisplayPos;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			const ImDrawIdx* idx_buffer_offset = 0;

			glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
			glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback)
				{
					// User callback (registered via ImDrawList::AddCallback)
					pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					ImVec4 clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y, pcmd->ClipRect.z - pos.x, pcmd->ClipRect.w - pos.y);
					if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
					{
						// Apply scissor/clipping rectangle
						glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

						// Bind texture, Draw
						glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
						glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
					}
				}
				idx_buffer_offset += pcmd->ElemCount;
			}
		}
		glDeleteVertexArrays(1, &vao_handle);

		// Restore modified GL state
		//glUseProgram(last_program);
		//glBindTexture(GL_TEXTURE_2D, last_texture);
		///*if (glBindSampler) */glBindSampler(0, last_sampler);
		//glActiveTexture(last_active_texture);
		//glBindVertexArray(last_vertex_array);
		//glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
		//glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
		//glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
		//if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		//if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		//if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		//if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		////glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
		//glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
		//glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);

		glfwSwapBuffers(m_pWindow);
	}
}
