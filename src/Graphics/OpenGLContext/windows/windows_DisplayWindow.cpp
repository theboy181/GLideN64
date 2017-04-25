#include <windows/GLideN64_Windows.h>
#include <GLideN64.h>
#include <Config.h>
#include <N64.h>
#include <RSP.h>
#include <FrameBuffer.h>
#include <GLideNUI/GLideNUI.h>
#include <DisplayWindow.h>
#include <Graphics/OpenGLContext/opengl_Wrapper.h>

using namespace opengl;

class DisplayWindowWindows : public DisplayWindow
{
public:
	DisplayWindowWindows() = default;

private:
	bool _start() override;
	void _stop() override;
	void _swapBuffers() override;
	void _saveScreenshot() override;
	bool _resizeWindow() override;
	void _changeWindow() override;
	void _readScreen(void **_pDest, long *_pWidth, long *_pHeight) override;
	void _readScreen2(void * _dest, int * _width, int * _height, int _front) override {}
};

DisplayWindow & DisplayWindow::get()
{
	static DisplayWindowWindows video;
	return video;
}

bool DisplayWindowWindows::_start()
{
#ifndef GL_DEBUG
	if (config.video.threadedVideo) {
		FunctionWrapper::setThreadedMode();
	}
#endif

	FunctionWrapper::windowsStart();
	return _resizeWindow();
}

void DisplayWindowWindows::_stop()
{
	FunctionWrapper::windowsStop();
}

void DisplayWindowWindows::_swapBuffers()
{
	FunctionWrapper::windowsSwapBuffers();
}

void DisplayWindowWindows::_saveScreenshot()
{
	unsigned char * pixelData = NULL;
	GLint oldMode;
	FunctionWrapper::glGetIntegerv(GL_READ_BUFFER, &oldMode);
	FunctionWrapper::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	FunctionWrapper::glReadBuffer(GL_FRONT);
	pixelData = (unsigned char*)malloc(m_screenWidth * m_screenHeight * 3);
	FunctionWrapper::glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelData);
	FunctionWrapper::glReadBuffer(oldMode);
	SaveScreenshot(m_strScreenDirectory, RSP.romname, m_screenWidth, m_screenHeight, pixelData);
	free( pixelData );
}

void DisplayWindowWindows::_changeWindow()
{
	static LONG		windowedStyle;
	static LONG		windowedExStyle;
	static RECT		windowedRect;
	static HMENU	windowedMenu;

	if (!m_bFullscreen) {
		DEVMODE fullscreenMode;
		memset( &fullscreenMode, 0, sizeof(DEVMODE) );
		fullscreenMode.dmSize = sizeof(DEVMODE);
		fullscreenMode.dmPelsWidth = config.video.fullscreenWidth;
		fullscreenMode.dmPelsHeight = config.video.fullscreenHeight;
		fullscreenMode.dmBitsPerPel = 32;
		fullscreenMode.dmDisplayFrequency = config.video.fullscreenRefresh;
		fullscreenMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;

		if (ChangeDisplaySettings( &fullscreenMode, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL) {
			MessageBox( NULL, L"Failed to change display mode", pluginNameW, MB_ICONERROR | MB_OK );
			return;
		}

		ShowCursor( FALSE );

		windowedMenu = GetMenu( hWnd );

		if (windowedMenu)
			SetMenu( hWnd, NULL );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_HIDE );

		windowedExStyle = GetWindowLong( hWnd, GWL_EXSTYLE );
		windowedStyle = GetWindowLong( hWnd, GWL_STYLE );

		SetWindowLong( hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST );
		SetWindowLong( hWnd, GWL_STYLE, WS_POPUP );

		GetWindowRect( hWnd, &windowedRect );

		m_bFullscreen = true;
		_resizeWindow();
	} else {
		ChangeDisplaySettings( NULL, 0 );

		ShowCursor( TRUE );

		if (windowedMenu)
			SetMenu( hWnd, windowedMenu );

		if (hStatusBar)
			ShowWindow( hStatusBar, SW_SHOW );

		SetWindowLong( hWnd, GWL_STYLE, windowedStyle );
		SetWindowLong( hWnd, GWL_EXSTYLE, windowedExStyle );
		SetWindowPos( hWnd, NULL, windowedRect.left, windowedRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE );

		m_bFullscreen = false;
		_resizeWindow();
	}
}

bool DisplayWindowWindows::_resizeWindow()
{
	RECT windowRect, statusRect, toolRect;

	if (m_bFullscreen) {
		m_screenWidth = config.video.fullscreenWidth;
		m_screenHeight = config.video.fullscreenHeight;
		m_heightOffset = 0;
		_setBufferSize();

		return (SetWindowPos(hWnd, NULL, 0, 0, m_screenWidth, m_screenHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW) == TRUE);
	} else {
		m_screenWidth = m_width = config.video.windowedWidth;
		m_screenHeight = config.video.windowedHeight;
		_setBufferSize();

		GetClientRect( hWnd, &windowRect );
		GetWindowRect( hStatusBar, &statusRect );

		if (hToolBar)
			GetWindowRect( hToolBar, &toolRect );
		else
			toolRect.bottom = toolRect.top = 0;

		m_heightOffset = (statusRect.bottom - statusRect.top);
		windowRect.right = windowRect.left + config.video.windowedWidth - 1;
		windowRect.bottom = windowRect.top + config.video.windowedHeight - 1 + m_heightOffset;

		AdjustWindowRect( &windowRect, GetWindowLong( hWnd, GWL_STYLE ), GetMenu( hWnd ) != NULL );

		return (SetWindowPos( hWnd, NULL, 0, 0, windowRect.right - windowRect.left + 1,
			windowRect.bottom - windowRect.top + 1 + toolRect.bottom - toolRect.top + 1, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE ) == TRUE);
	}
}

void DisplayWindowWindows::_readScreen(void **_pDest, long *_pWidth, long *_pHeight)
{
	*_pWidth = m_width;
	*_pHeight = m_height;

	*_pDest = malloc(m_height * m_width * 3);
	if (*_pDest == nullptr)
		return;

#ifndef GLESX
	GLint oldMode;
	FunctionWrapper::glGetIntegerv(GL_READ_BUFFER, &oldMode);
	FunctionWrapper::glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	FunctionWrapper::glReadBuffer(GL_FRONT);
	FunctionWrapper::glReadPixels(0, m_heightOffset, m_width, m_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, *_pDest);
	FunctionWrapper::glReadBuffer(oldMode);
#else
	FunctionWrapper::glReadPixels(0, m_heightOffset, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, *_pDest);
#endif
}
