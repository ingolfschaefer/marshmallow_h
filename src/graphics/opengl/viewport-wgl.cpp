/*
 * Copyright 2011 Marshmallow Engine. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list
 *      of conditions and the following disclaimer in the documentation and/or other materials
 *      provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHMALLOW ENGINE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MARSHMALLOW ENGINE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Marshmallow Engine.
 */

#include "graphics/viewport.h"

/*!
 * @file
 *
 * @author Guillermo A. Amaral B. (gamaral) <g@maral.me>
 */

#include <windows.h>

#include <GL/gl.h>

#include "core/logger.h"
#include "event/eventmanager.h"
#include "event/keyboardevent.h"
#include "event/quitevent.h"
#include "graphics/painter.h"

MARSHMALLOW_NAMESPACE_USE;
using namespace Graphics;

const Core::Type Viewport::sType("WGL");

struct Viewport::Internal
{
	HDC   dcontext;
	HGLRC context;
	HWND  window;
	RECT  wrect;
	float camera[3];
	float size[2];
	float vscale;
	bool  fullscreen;
	bool  loaded;

	Internal(void)
	    : dcontext(0),
	      context(0),
	      window(0),
	      vscale(1),
	      loaded(false)
	{
		wrect.bottom = wrect.left = wrect.top = wrect.bottom = 0;

		camera[0] = camera[1] = 0.f;  // camera x y
		camera[2] = 1.f;              // camera zoom

		size[0] = size[1] = 0.f;
	}

	static LRESULT CALLBACK
	WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg) {
		case WM_CLOSE: {
			Event::QuitEvent event(-1);
			Event::EventManager::Instance()->dispatch(event);
			return(0);
		} break;

		case WM_SIZE:
			return(0);
		break;

		case WM_SYSCOMMAND:
			switch (wParam) {
			case SC_MONITORPOWER:
			case SC_SCREENSAVE:
				if (MVI.fullscreen)
					return(0);
			break;
			}
		break;

		case WM_KEYDOWN:
			MVI.handleKeyEvent( static_cast<int>(wParam), true );
			break;
		case WM_KEYUP:
			MVI.handleKeyEvent( static_cast<int>(wParam), false );
			break;
		}

		return(DefWindowProc(hwnd, uMsg, wParam, lParam));
	}

	bool
	createWindow(int w, int h, int d, bool f)
	{
		loaded = false;
		fullscreen = f;
		context = 0;
		dcontext = 0;
		window = 0;

		WNDCLASS l_wc;
		l_wc.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
		l_wc.lpfnWndProc   = (WNDPROC) WindowProc;
		l_wc.cbClsExtra    = 0;
		l_wc.cbWndExtra    = 0;
		l_wc.hInstance     = GetModuleHandle(0);
		l_wc.hIcon         = 0;
		l_wc.hCursor       = LoadCursor(0, IDC_ARROW);
		l_wc.hbrBackground = 0;
		l_wc.lpszMenuName  = 0;
		l_wc.lpszClassName = "marshmallow_wgl";

		if (!(RegisterClass(&l_wc))) {
			ERROR("Failed to register window class.");
			return false;
		}
	
		if (fullscreen) {
			DEVMODE l_dm;
			memset(&l_dm, 0, sizeof(l_dm));

			l_dm.dmSize       = sizeof(l_dm);
			l_dm.dmPelsWidth  = w;
			l_dm.dmPelsHeight = h;
			l_dm.dmBitsPerPel = d;
			l_dm.dmFields     = DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

			if (ChangeDisplaySettings(&l_dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
				ERROR("Failed to switch modes for fullscreen viewport");
				fullscreen = false;
			} else {
				ShowCursor(false);
			}
		}

		DWORD l_wstyle[2];
		l_wstyle[0] = WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
		l_wstyle[1] = WS_EX_APPWINDOW;
		if (fullscreen) l_wstyle[0] |= WS_POPUP;
		else {
			l_wstyle[0] |= WS_OVERLAPPEDWINDOW;
			l_wstyle[1] |= WS_EX_WINDOWEDGE;
		}

		/* get desktop size */
		RECT l_warea;
		if (!SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT), &l_warea, 0)) {
			l_warea.left   = l_warea.top = 0;
			l_warea.right  = GetSystemMetrics(SM_CXSCREEN);
			l_warea.bottom = GetSystemMetrics(SM_CYSCREEN);
		}

		/* create window rect */
		wrect.left   = ((l_warea.right - l_warea.left) - w) / 2;
		wrect.right  = wrect.left + w;
		wrect.top    = ((l_warea.bottom - l_warea.top) - h) / 2;
		wrect.bottom = wrect.top + h;
		AdjustWindowRectEx(&wrect, l_wstyle[0], false, l_wstyle[1]);

		/* create actual window */
		window = CreateWindowEx(l_wstyle[1], "marshmallow_wgl", 0, l_wstyle[0],
		    wrect.left, wrect.top, wrect.right - wrect.left,
		    wrect.bottom - wrect.top, 0, 0, l_wc.hInstance, 0);
		if (!window) {
			ERROR1("Failed to create window");
			destroyWindow();
			return(false);
		}

		/* create device context */
		if (!(dcontext = GetDC(window))) {
			ERROR1("Failed to create device context");
			destroyWindow();
			return(false);
		}

		/* select pixel format */
		static PIXELFORMATDESCRIPTOR l_pfd;
		memset(&l_pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		l_pfd.nSize      = sizeof(PIXELFORMATDESCRIPTOR);
		l_pfd.nVersion   = 1;
		l_pfd.dwFlags    = PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
		l_pfd.iPixelType = PFD_TYPE_RGBA;
		l_pfd.cColorBits = d;
		l_pfd.cDepthBits = 16;
		l_pfd.iLayerType = PFD_MAIN_PLANE;
	
		unsigned int l_pfindex;
		if (!(l_pfindex = ChoosePixelFormat(dcontext, &l_pfd))) {
			ERROR1("Failed to select a pixel format");
			destroyWindow();
			return(false);
		}

		if(!SetPixelFormat(dcontext, l_pfindex, &l_pfd)) {
			ERROR1("Failed to bind pixel format to window context");
			destroyWindow();
			return(false);
		}

		/* create wiggle context */
		if (!(context = wglCreateContext(dcontext))) {
			ERROR1("Failed to create OpenGL context");
			destroyWindow();
			return(false);
		}

		/* make wgl context current */
		if(!wglMakeCurrent(dcontext, context)) {
			ERROR1("Failed to set current OpenGL context");
			destroyWindow();
			return(false);
		}

		ShowWindow(window, SW_SHOW);
		SetForegroundWindow(window);
		SetFocus(window);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);

		/* initialize context */

		glViewport(0, 0, w, h);
		glClearColor(0., 0., 0., 0.);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		adjustView();
		SwapBuffer();

		if( glGetError() != GL_NO_ERROR )
			return(false);

		return(loaded = true);
	}

	void
	destroyWindow(void)
	{
		if (fullscreen) {
			ShowCursor(true);
			ChangeDisplaySettings(0, 0);
		}

		if (context) {
			if (!wglMakeCurrent(0, 0))
				WARNING1("Failed to unset current OpenGL context");

			if (!wglDeleteContext(context))
				WARNING1("Failed to delete OpenGL context");

			context = 0;
		}

		if (dcontext && !ReleaseDC(window, dcontext))
			WARNING1("Failed to release device context");
		dcontext = 0;

		if (window && !DestroyWindow(window))
			WARNING1("Failed to destroy window");
		window = 0;

		if (!UnregisterClass("marshmallow_wgl", GetModuleHandle(0)))
			WARNING1("Failed to unregister window class");

		loaded = false;
	}

	void
	adjustView(void)
	{
		const float aratio =
		    (static_cast<float>(wrect.bottom - wrect.top) /
		     static_cast<float>(wrect.right - wrect.left));

		size[0] = DEFAULT_VIEWPORT_VWIDTH * camera[2];
		size[1] = (DEFAULT_VIEWPORT_VWIDTH * aratio) * camera[2];
		vscale = static_cast<float>(wrect.right - wrect.left) / size[0];

		const float l_hw = size[0] / 2.f;
		const float l_hh = size[1] / 2.f;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-l_hw + camera[0], l_hw + camera[0], -l_hh + camera[1], l_hh + camera[1], -1.f, 1.f);
		glMatrixMode(GL_MODELVIEW);
	}
	
	void
	handleKeyEvent( int keyCode, bool keyPressed )
	{
		Event::KBKeys l_key = Event::KEY_NONE;
		Event::KBActions l_action =
		    (keyPressed ? Event::KeyPressed : Event::KeyReleased);

		// win32 virtual key codes for alphanumerical chars correspond to the char itself
		if ( (keyCode >= '0' && keyCode <= '9') ||
			 (keyCode >= 'a' && keyCode <= 'z') ) {
			l_key = static_cast<Event::KBKeys>(keyCode);
		} else {
			switch (keyCode) {
//			case XK_Alt_L:        l_key = Event::KEY_ALT_L; break;
//			case XK_Alt_R:        l_key = Event::KEY_ALT_R; break;
			case VK_BACK:         l_key = Event::KEY_BACKSPACE; break;
			case VK_CAPITAL:      l_key = Event::KEY_CAPS_LOCK; break;
			case VK_CLEAR:        l_key = Event::KEY_CLEAR; break;
			case VK_LCONTROL:     l_key = Event::KEY_CONTROL_R; break;
			case VK_RCONTROL:     l_key = Event::KEY_CONTROL_L; break;
			case VK_DELETE:       l_key = Event::KEY_DELETE; break;
			case VK_DOWN:         l_key = Event::KEY_DOWN; break;
			case VK_END:          l_key = Event::KEY_END; break;
			case VK_ESCAPE:       l_key = Event::KEY_ESCAPE; break;
			case VK_HELP:         l_key = Event::KEY_HELP; break;
			case VK_HOME:         l_key = Event::KEY_HOME; break;
			case VK_INSERT:       l_key = Event::KEY_INSERT; break;
			case VK_LEFT:         l_key = Event::KEY_LEFT; break;
			case VK_MENU:         l_key = Event::KEY_MENU; break;
			case VK_NUMLOCK:      l_key = Event::KEY_NUM_LOCK; break;
			case VK_NEXT:         l_key = Event::KEY_PAGE_DOWN; break;
			case VK_PRIOR:        l_key = Event::KEY_PAGE_UP; break;
			case VK_PAUSE:        l_key = Event::KEY_PAUSE; break;
			case VK_PRINT:        l_key = Event::KEY_PRINT; break;
			case VK_RETURN:       l_key = Event::KEY_RETURN; break;
			case VK_RIGHT:        l_key = Event::KEY_RIGHT; break;
			case VK_SCROLL:       l_key = Event::KEY_SCROLL_LOCK; break;
			case VK_LSHIFT:       l_key = Event::KEY_SHIFT_L; break;
			case VK_RSHIFT:       l_key = Event::KEY_SHIFT_R; break;
			case VK_TAB:          l_key = Event::KEY_TAB; break;
			case VK_UP:           l_key = Event::KEY_UP; break;
//			case XK_backslash:    l_key = Event::KEY_BACKSLASH; break;
//			case XK_bracketleft:  l_key = Event::KEY_BRACKETLEFT; break;
//			case XK_bracketright: l_key = Event::KEY_BRACKETRIGHT; break;
//			case XK_equal:        l_key = Event::KEY_EQUAL; break;
//			case XK_less:         l_key = Event::KEY_LESS; break;
//			case XK_quotedbl:     l_key = Event::KEY_DBLQUOTE; break;
//			case XK_semicolon:    l_key = Event::KEY_SEMICOLON; break;
			case VK_SPACE:        l_key = Event::KEY_SPACE; break;
			case VK_F1:           l_key = Event::KEY_F1; break;
			case VK_F2:           l_key = Event::KEY_F2; break;
			case VK_F3:           l_key = Event::KEY_F3; break;
			case VK_F4:           l_key = Event::KEY_F4; break;
			case VK_F5:           l_key = Event::KEY_F5; break;
			case VK_F6:           l_key = Event::KEY_F6; break;
			case VK_F7:           l_key = Event::KEY_F7; break;
			case VK_F8:           l_key = Event::KEY_F8; break;
			case VK_F9:           l_key = Event::KEY_F9; break;
			case VK_F10:          l_key = Event::KEY_F10; break;
			case VK_F11:          l_key = Event::KEY_F11; break;
			case VK_F12:          l_key = Event::KEY_F12; break;
			case VK_F13:          l_key = Event::KEY_F13; break;
			case VK_F14:          l_key = Event::KEY_F14; break;
			case VK_F15:          l_key = Event::KEY_F15; break;
			case VK_NUMPAD0:      l_key = Event::KEY_K0; break;
			case VK_NUMPAD1:      l_key = Event::KEY_K1; break;
			case VK_NUMPAD2:      l_key = Event::KEY_K2; break;
			case VK_NUMPAD3:      l_key = Event::KEY_K3; break;
			case VK_NUMPAD4:      l_key = Event::KEY_K4; break;
			case VK_NUMPAD5:      l_key = Event::KEY_K5; break;
			case VK_NUMPAD6:      l_key = Event::KEY_K6; break;
			case VK_NUMPAD7:      l_key = Event::KEY_K7; break;
			case VK_NUMPAD8:      l_key = Event::KEY_K8; break;
			case VK_NUMPAD9:      l_key = Event::KEY_K9; break;
			case VK_DECIMAL:      l_key = Event::KEY_KDECIMAL; break;
			case VK_DIVIDE:       l_key = Event::KEY_KDIVIDE; break;
			case VK_MULTIPLY:     l_key = Event::KEY_KMULTIPLY; break;
			default: WARNING1("Unknown key pressed!");
			}
		}

		Event::SharedEvent event(new Event::KeyboardEvent(l_key, l_action));
		Event::EventManager::Instance()->queue(event);
	}

} MVI;


bool
Viewport::Initialize(int w, int h, int d, bool f)
{
	bool l_success = MVI.createWindow(w, h, d, f);

	if (l_success) Painter::Initialize();

	return(l_success);
}

void
Viewport::Finalize(void)
{
	Painter::Finalize();
	MVI.destroyWindow();
}

bool
Viewport::Redisplay(int w, int h, int d, bool f)
{
	MVI.destroyWindow();
	return(MVI.createWindow(w, h, d, f));
}

void
Viewport::Tick(TIME t)
{
	TIMEOUT_INIT;

	MSG l_msg;
	while (PeekMessage(&l_msg, 0, 0, 0, PM_NOREMOVE)) {
		if (l_msg.message == WM_QUIT) {
			Event::QuitEvent event(-1);
			Event::EventManager::Instance()->dispatch(event);
			break;
		}

		if (GetMessage(&l_msg, 0, 0, 0) > 0) {
			TranslateMessage(&l_msg);
			DispatchMessage(&l_msg);
		}
	}

}

void
Viewport::SwapBuffer(void)
{
	SwapBuffers(MVI.dcontext);
	glClearColor(.0, .0, .0, .0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
}

const Math::Vector3
Viewport::Camera(void)
{
	return(Math::Vector3(MVI.camera[0], MVI.camera[1], MVI.camera[2]));
}

void
Viewport::SetCamera(const Math::Vector3 &c)
{
	MVI.camera[0] = c[0];
	MVI.camera[1] = c[1];
	MVI.camera[2] = c[2];

	MVI.adjustView();
}

const Math::Size2
Viewport::Size(void)
{
	return(Math::Size2(MVI.size[0], MVI.size[1]));
}

const Math::Size2
Viewport::WindowSize(void)
{
	if (MVI.loaded)
		return(Math::Size2(static_cast<float>(MVI.wrect.right - MVI.wrect.left),
		                   static_cast<float>(MVI.wrect.bottom - MVI.wrect.top)));

	return(Math::Size2());
}

float
Viewport::MapToWorld(int x)
{
	return(static_cast<float>(x) / MVI.vscale);
}

int
Viewport::MapFromWorld(float x)
{
	return(static_cast<int>(floor(static_cast<float>(x) * MVI.vscale)));
}

const Core::Type &
Viewport::Type(void)
{
	return(sType);
}
