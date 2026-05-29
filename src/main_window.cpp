
#include <Windows.h>
#include <CommCtrl.h>

#include "main_window.h"

#include "win_filesystem.h"
#include "win_dialogue.h"
#include "native-ui/media_setting_dialogue.h"
#include "native-ui/window_menu.h"
#include "text_utility.h"
#include "path_utility.h"

CMainWindow::CMainWindow()
{

}

CMainWindow::~CMainWindow()
{

}

bool CMainWindow::create(HINSTANCE hInstance, const wchar_t* windowName)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = &WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszClassName = m_className;

	if (::RegisterClassExW(&wcex))
	{
		m_hInstance = hInstance;

		UINT dpi = ::GetDpiForSystem();
		int windowWidth = ::MulDiv(200, dpi, USER_DEFAULT_SCREEN_DPI);
		int windowHeight = ::MulDiv(200, dpi, USER_DEFAULT_SCREEN_DPI);

		m_hWnd = ::CreateWindowW(m_className, windowName, WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, hInstance, this);
		if (m_hWnd != nullptr)
		{
			return true;
		}
		else
		{
			showErrorMessageBox(L"CreateWindowExW failed; code: %ul", ::GetLastError());
		}
	}
	else
	{
		showErrorMessageBox(L"RegisterClassW failed; code: %ul", ::GetLastError());
	}

	return false;
}

int CMainWindow::messageLoop()
{
	MSG msg;

	for (;;)
	{
		BOOL bRet = ::GetMessageW(&msg, 0, 0, 0);
		if (bRet > 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
		else if (bRet == 0)
		{
			/* ループ終了 */
			return static_cast<int>(msg.wParam);
		}
		else
		{
			/* ループ異常 */
			showErrorMessageBox(L"GetMessageW failed; code: %ul", ::GetLastError());
			return -1;
		}
	}
	return 0;
}
/* C CALLBACK */
LRESULT CMainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMainWindow* pThis = nullptr;
	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = reinterpret_cast<CMainWindow*>(pCreateStruct->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	}

	pThis = reinterpret_cast<CMainWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pThis != nullptr)
	{
		return pThis->handleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/* メッセージ処理 */
LRESULT CMainWindow::handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return onCreate(hWnd);
	case WM_DESTROY:
		return onDestroy();
	case WM_CLOSE:
		return onClose();
	case WM_PAINT:
		return onPaint();
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		return onKeyDown(wParam, lParam);
	case WM_KEYUP:
		return onKeyUp(wParam, lParam);
	case WM_COMMAND:
		return onCommand(wParam, lParam);
	case WM_TIMER:
		return onTimer(wParam);
	case WM_MOUSEMOVE:
		return onMouseMove(wParam, lParam);
	case WM_MOUSEWHEEL:
		return onMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return onLButtonDown(wParam, lParam);
	case WM_LBUTTONUP:
		return onLButtonUp(wParam, lParam);
	case WM_RBUTTONUP:
		return onRButtonUp(wParam, lParam);
	case WM_MBUTTONUP:
		return onMButtonUp(wParam, lParam);
	case EventMessage::kAudioPlayer:
		onAudioPlayerEvent(static_cast<unsigned long>(lParam));
		break;
	default:
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/* WM_CREATE */
LRESULT CMainWindow::onCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	initialiseMenuBar();

	m_pD2ImageDrawer = std::make_unique<CD2ImageDrawer>(m_hWnd);

	m_pD2TextWriter = std::make_unique<CD2TextWriter>(m_pD2ImageDrawer->getD2Factory(), m_pD2ImageDrawer->getD2DeviceContext());
	m_pD2TextWriter->setupOutLinedDrawing(L"C:\\Windows\\Fonts\\yumindb.ttf");

	m_pViewManager = std::make_unique<CViewManager>(m_hWnd);

	m_pMfVoicePlayer = std::make_unique<CMfMediaPlayer>();
	m_pMfVoicePlayer->setPlaybackWindow(m_hWnd, EventMessage::kAudioPlayer);

	m_pSceneCrafter = std::make_unique<CLilyanSceneCrafter>(m_pD2ImageDrawer->getD2DeviceContext());
	setMenuCheckState(MenuBar::kImage, Menu::kSyncImage, m_pSceneCrafter->isImageSynced());

	m_pFontSettingDialogue = std::make_unique<CFontSettingDialogue>();

	return 0;
}
/* WM_DESTROY */
LRESULT CMainWindow::onDestroy()
{
	::PostQuitMessage(0);

	return 0;
}
/* WM_CLOSE */
LRESULT CMainWindow::onClose()
{
	::KillTimer(m_hWnd, Timer::kText);

	::DestroyWindow(m_hWnd);
	::UnregisterClassW(m_className, m_hInstance);

	return 0;
}
/* WM_PAINT */
LRESULT CMainWindow::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	if (m_pD2ImageDrawer == nullptr || m_pD2TextWriter == nullptr || m_pViewManager == nullptr || m_pSceneCrafter == nullptr)
	{
		::EndPaint(m_hWnd, &ps);
		return 0;
	}

	m_pD2ImageDrawer->clear();

	ID2D1Bitmap* pD2d1Bitmap = m_pSceneCrafter->getCurrentImage();
	if (pD2d1Bitmap != nullptr)
	{
		RECT rc;
		::GetClientRect(m_hWnd, &rc);

		int targetWidth = rc.right - rc.left;
		int targetHeight = rc.bottom - rc.top;

		D2D1_SIZE_U srcSize = pD2d1Bitmap->GetPixelSize();

		const float fScale = m_pViewManager->getScale();
		const float fX = (srcSize.width * fScale - targetWidth) / 2 + m_pViewManager->offsetX() / 2;
		const float fY = (srcSize.height * fScale - targetHeight) / 2 + m_pViewManager->offsetY() / 2;

		const D2D1_MATRIX_3X2_F scaleMatrix = D2D1::Matrix3x2F::Scale(fScale, fScale);
		const D2D1_MATRIX_3X2_F translateMatrix = D2D1::Matrix3x2F::Translation(-fX, -fY);
		const D2D1_MATRIX_3X2_F transformMatrix = scaleMatrix * translateMatrix;

		m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(transformMatrix);
		m_pD2ImageDrawer->draw(pD2d1Bitmap);
		m_pD2ImageDrawer->getD2DeviceContext()->SetTransform(D2D1::Matrix3x2F::Identity());

		if (!m_isTextHidden)
		{
			const std::wstring& message = m_pSceneCrafter->getCurrentFormattedText();
			m_pD2TextWriter->outLinedDraw(message.c_str(), message.size());
		}
		m_pD2ImageDrawer->display();
	}

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::onSize()
{
	return 0;
}
/*WM_KEYDOWN*/
LRESULT CMainWindow::onKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		autoTexting();
		break;
	case VK_LEFT:
		shiftText(false);
		break;
	default:

		break;
	}

	return 0;
}
/*WM_KEYUP*/
LRESULT CMainWindow::onKeyUp(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_ESCAPE:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	case VK_UP:
		menuOnForeFile();
		break;
	case VK_DOWN:
		menuOnNextFile();
		break;
	case 'C':
		if (m_pD2TextWriter.get() != nullptr)
		{
			m_pD2TextWriter->toggleTextColour();
			updateScreen();
		}
		break;
	case 'T':
		m_isTextHidden ^= true;
		updateScreen();
		break;
	}
	return 0;
}
/*WM_COMMAND*/
LRESULT CMainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);
	int msgSource = LOWORD(lParam);
	if (msgSource == 0)
	{
		/* Menus */
		switch (id)
		{
		case Menu::kOpenFile:
			menuOnOpenFile();
			break;
		case Menu::kVoiceSetting:
			menuOnVoiceSetting();
			break;
		case Menu::kSoundSetting:
			menuOnSoundSetting();
			break;
		case Menu::kFontSetting:
			menuOnFontSetting();
			break;
		case Menu::kSyncImage:
			menuOnSyncImage();
			break;
		default:
			if (id >= Menu::kLabelStartIndex)
			{
				jumpScene(static_cast<size_t>(id - Menu::kLabelStartIndex));
			}
			break;
		}
	}
	else
	{
		/* Controls */
	}

	return 0;
}
/*WM_TIMER*/
LRESULT CMainWindow::onTimer(WPARAM wParam)
{
	switch (wParam)
	{
	case Timer::kText:
		if (m_pMfVoicePlayer != nullptr)
		{
			if (m_pMfVoicePlayer->isEnded())
			{
				autoTexting();
			}
		}
		break;
	default:
		break;
	}
	return 0;
}
/* WM_MOUSEMOVE */
LRESULT CMainWindow::onMouseMove(WPARAM wParam, LPARAM lParam)
{
	WORD pressedKey = LOWORD(wParam);
	if (pressedKey == MK_LBUTTON)
	{
		if (m_mouseState.wasRightCombined)return 0;

		POINT pt{};
		::GetCursorPos(&pt);
		int iX = m_mouseState.lastMousePos.x - pt.x;
		int iY = m_mouseState.lastMousePos.y - pt.y;

		if (m_mouseState.hasLeftBeenDragged)
		{
			if (m_pViewManager != nullptr)
			{
				m_pViewManager->addOffset(iX, iY);
				updateScreen();
			}
		}

		m_mouseState.lastMousePos = pt;
		m_mouseState.hasLeftBeenDragged = true;
	}

	return 0;
}
/* WM_MOUSEWHEEL */
LRESULT CMainWindow::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	short scroll = -static_cast<short>(HIWORD(wParam)) / WHEEL_DELTA;
	WORD pressedKey = LOWORD(wParam);

	if (pressedKey == 0)
	{
		if (m_pSceneCrafter.get() != nullptr && m_pSceneCrafter->hasScenarioData())
		{
			if (m_pViewManager.get() != nullptr)
			{
				m_pViewManager->rescale(scroll > 0);
			}
		}
	}
	else if (pressedKey == MK_LBUTTON)
	{

	}
	else if (pressedKey == MK_RBUTTON)
	{
		shiftText(scroll > 0);

		m_mouseState.wasRightCombined = true;
	}

	return 0;
}
/* WM_LBUTTONDOWN */
LRESULT CMainWindow::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_mouseState.lastMousePos);

	m_mouseState.wasLeftPressed = true;

	return 0;
}
/* WM_LBUTTONUP */
LRESULT CMainWindow::onLButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD pressedKey = LOWORD(wParam);

	if (pressedKey == MK_RBUTTON && m_isBarHidden)
	{
		::PostMessage(m_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
		INPUT input{};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_DOWN;
		::SendInput(1, &input, sizeof(input));

		m_mouseState.wasRightCombined = true;
	}

	if (pressedKey == 0)
	{
		if (m_mouseState.wasLeftPressed)
		{
			POINT pt{};
			::GetCursorPos(&pt);
			int iX = m_mouseState.lastMousePos.x - pt.x;
			int iY = m_mouseState.lastMousePos.y - pt.y;

			if (iX == 0 && iY == 0)
			{
				if (m_pSceneCrafter.get() != nullptr)
				{
					if (!m_pSceneCrafter->isImageSynced())
					{
						m_pSceneCrafter->shiftForwardImage();
						updateScreen();
					}
				}
			}
		}
	}

	m_mouseState.wasLeftPressed = false;

	return 0;
}
/* WM_RBUTTONUP */
LRESULT CMainWindow::onRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_mouseState.wasRightCombined)
	{
		m_mouseState.wasRightCombined = false;
		return 0;
	}

	WORD pressedKey = LOWORD(wParam);
	if (pressedKey == 0)
	{
		if (m_pSceneCrafter.get() != nullptr && m_pSceneCrafter->hasScenarioData())
		{
			const auto& labelData = m_pSceneCrafter->getLabelData();
			if (labelData.empty())return 0;

			HMENU hPopupMenu = ::CreatePopupMenu();
			if (hPopupMenu != nullptr)
			{
				for (size_t i = 0; i < labelData.size(); ++i)
				{
					::AppendMenuW(hPopupMenu, MF_STRING, Menu::kLabelStartIndex + i, labelData[i].caption.c_str());
				}

				POINT point{};
				::GetCursorPos(&point);
				::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON, point.x, point.y, 0, m_hWnd, nullptr);
				::DestroyMenu(hPopupMenu);
			}
		}
	}

	return 0;
}
/* WM_MBUTTONUP */
LRESULT CMainWindow::onMButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD pressedKey = LOWORD(wParam);
	if (pressedKey == 0)
	{
		if (m_pSceneCrafter.get() != nullptr && m_pSceneCrafter->hasScenarioData())
		{
			if (m_pViewManager.get() != nullptr)
			{
				m_pViewManager->resetScale();
			}
		}
	}
	else if (pressedKey == MK_RBUTTON)
	{
		toggleWindowBorderStyle();

		m_mouseState.wasRightCombined = true;
	}

	return 0;
}
/* 操作欄作成 */
void CMainWindow::initialiseMenuBar()
{
	if (m_hMenuBar != nullptr)return;

	HMENU hMenu = window_menu::MenuBuilder(
		{
			{0, L"File", window_menu::MenuBuilder(
				{
					{ Menu::kOpenFile, L"Open"},
				}).get()
			},
			{0, L"Setting", window_menu::MenuBuilder(
				{
					{ Menu::kVoiceSetting, L"Voice"},
					{ Menu::kSoundSetting, L"Sound"},
					{ Menu::kFontSetting, L"Font"}
				}).get()
			},
			{0, L"Image", window_menu::MenuBuilder(
				{
					{ Menu::kSyncImage, L"Sync"},
				}).get()
			}
		}
	).get();

	if (::IsMenu(hMenu))
	{
		if (::SetMenu(m_hWnd, hMenu))
		{
			m_hMenuBar = hMenu;
		}
		else
		{
			showErrorMessageBox(L"Failed to create menu; code: %ul", ::GetLastError());
			::DestroyMenu(hMenu);
		}
	}
}

void CMainWindow::showErrorMessageBox(const wchar_t* format, ...) const
{
	wchar_t messageBuffer[512]{};
	constexpr size_t bufferSize = sizeof(messageBuffer) / sizeof(wchar_t) - 1;

	va_list args;
	va_start(args, format);
	vswprintf_s(messageBuffer, bufferSize, format, args);
	va_end(args);

	/* m_hWnd might be null. */
	::MessageBoxW(m_hWnd, messageBuffer, L"Error", MB_ICONERROR);
}
/* ファイル選択 */
void CMainWindow::menuOnOpenFile()
{
	constexpr wchar_t fileFilter[] = L"chara*_20*.nani";
	std::wstring selectedFilePath = win_dialogue::SelectOpenFile(L"Script file", fileFilter, nullptr, m_hWnd);
	if (!selectedFilePath.empty())
	{
		bool bRet = setupScenario(selectedFilePath);
		if (bRet)
		{
			m_scriptFilePaths.clear();
			m_nScriptFileIndex = 0;
			win_filesystem::GetFilePathListAndIndex(selectedFilePath, fileFilter, m_scriptFilePaths, m_nScriptFileIndex);
		}
	}
}
/* 次のファイルに移動 */
void CMainWindow::menuOnNextFile()
{
	if (m_scriptFilePaths.empty())return;

	++m_nScriptFileIndex;
	if (m_nScriptFileIndex >= m_scriptFilePaths.size())m_nScriptFileIndex = 0;

	setupScenario(m_scriptFilePaths[m_nScriptFileIndex]);
}
/*前のファイルに移動*/
void CMainWindow::menuOnForeFile()
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptFileIndex;
	if (m_nScriptFileIndex >= m_scriptFilePaths.size())m_nScriptFileIndex = m_scriptFilePaths.size() - 1;

	setupScenario(m_scriptFilePaths[m_nScriptFileIndex]);
}
/* 音声音量・再生速度変更 */
void CMainWindow::menuOnVoiceSetting()
{
	if (m_pMfVoicePlayer.get() != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.open(m_hInstance, m_hWnd, m_pMfVoicePlayer.get(), L"Voice");
	}
}
/* 効果音音量・再生速度変更 */
void CMainWindow::menuOnSoundSetting()
{
	if (m_pMfSoundPlayer.get() != nullptr)
	{
		CMediaSettingDialogue mediaSettingDialogue;
		mediaSettingDialogue.open(m_hInstance, m_hWnd, m_pMfSoundPlayer.get(), L"Sound");
	}
}

void CMainWindow::menuOnFontSetting()
{
	if (m_pFontSettingDialogue != nullptr)
	{
		if (m_pFontSettingDialogue->getHwnd() == nullptr)
		{
			HWND hWnd = m_pFontSettingDialogue->open(m_hInstance, m_hWnd, L"Font", m_pD2TextWriter.get());
			::ShowWindow(hWnd, SW_SHOWNORMAL);
		}
		else
		{
			::SetFocus(m_pFontSettingDialogue->getHwnd());
		}
	}
}

void CMainWindow::menuOnSyncImage()
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		bool bRet = setMenuCheckState(MenuBar::kImage, Menu::kSyncImage, !m_pSceneCrafter->isImageSynced());
		if (bRet)
		{
			m_pSceneCrafter->syncImage(!m_pSceneCrafter->isImageSynced());
			updateScreen();
		}
	}
}
/* 表示形式切り替え */
void CMainWindow::toggleWindowBorderStyle()
{
	if (m_pSceneCrafter.get() == nullptr || !m_pSceneCrafter->hasScenarioData())return;

	RECT rect;
	::GetWindowRect(m_hWnd, &rect);
	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	m_isBarHidden ^= true;

	if (m_isBarHidden)
	{
		MONITORINFO monitorInfo{ .cbSize = sizeof(MONITORINFO) };
		if (HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST); hMonitor != nullptr)
		{
			[[maybe_unused]] BOOL iRet = ::GetMonitorInfoW(hMonitor, &monitorInfo);
		}

		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle & ~WS_CAPTION & ~WS_SYSMENU);
		::SetWindowPos(m_hWnd, nullptr, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		::SetMenu(m_hWnd, nullptr);
	}
	else
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle | WS_CAPTION | WS_SYSMENU);
		::SetMenu(m_hWnd, m_hMenuBar);
	}

	if (m_pViewManager != nullptr)
	{
		m_pViewManager->onStyleChanged();
	}
}
/* 印状態変更 */
bool CMainWindow::setMenuCheckState(unsigned int uiMenuIndex, unsigned int uiItemIndex, bool checked) const
{
	HMENU hMenuBar = ::GetMenu(m_hWnd);
	if (hMenuBar != nullptr)
	{
		HMENU hMenu = ::GetSubMenu(hMenuBar, uiMenuIndex);
		if (hMenu != nullptr)
		{
			DWORD ulRet = ::CheckMenuItem(hMenu, uiItemIndex, checked ? MF_CHECKED : MF_UNCHECKED);
			return ulRet != (DWORD)-1;
		}
	}

	return false;
}
/* 寸劇構築 */
bool CMainWindow::setupScenario(const std::wstring& filePath)
{
	bool bRet = false;

	if (m_pSceneCrafter.get() != nullptr)
	{
		bRet = m_pSceneCrafter->loadScenario(filePath);
		if (bRet)
		{
			unsigned int width = 0, height = 0;
			m_pSceneCrafter->getCurrentImageSize(&width, &height);

			if (m_pViewManager != nullptr)
			{
				m_pViewManager->setBaseSize(width, height);
				m_pViewManager->resetScale();
			}

			std::wstring windowTitle = m_pSceneCrafter->getSceneTitle();
			windowTitle.append(L"; ").append(path_utility::ExtractFileNameWithoutExtension(filePath));
			::SetWindowText(m_hWnd, windowTitle.data());

			m_pMfSoundPlayer = std::make_unique<CMfMediaPlayer>();

			updateText();
		}
		else
		{
			showErrorMessageBox(L"Failed to set up %.*s", static_cast<int>(filePath.size()), filePath.data());
		}
	}

	return bRet;
}

void CMainWindow::jumpScene(size_t nIndex)
{
	if (m_pSceneCrafter.get() != nullptr && m_pSceneCrafter->hasScenarioData())
	{
		m_pSceneCrafter->jumpToLabel(nIndex);
		updateText();
	}
}
/*再描画要求*/
void CMainWindow::updateScreen()
{
	::InvalidateRect(m_hWnd, nullptr, FALSE);
}
/*文章送り・戻し*/
void CMainWindow::shiftText(bool forward)
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		m_pSceneCrafter->shiftScene(forward);
		updateText();
	}
}
/*文章更新*/
void CMainWindow::updateText()
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		if (m_pMfSoundPlayer.get() != nullptr)
		{
			const wchar_t* pSoundFilePath = m_pSceneCrafter->getCurrentSoundFilePath();
			if (pSoundFilePath != nullptr && *pSoundFilePath != L'\0')
			{
				m_pMfSoundPlayer->play(pSoundFilePath);
			}
		}

		if (m_pMfVoicePlayer.get() != nullptr)
		{
			const wchar_t* pVoiceFilePath = m_pSceneCrafter->getCurrentVoiceFilePath();
			if (pVoiceFilePath != nullptr && *pVoiceFilePath != L'\0')
			{
				m_pMfVoicePlayer->play(pVoiceFilePath);
			}
		}

		constexpr unsigned int kTimerInterval = 2000;
		::SetTimer(m_hWnd, Timer::kText, kTimerInterval, nullptr);

		updateScreen();
	}
}
/* IMFMediaEngineNotify::EventNotify */
void CMainWindow::onAudioPlayerEvent(unsigned long ulEvent)
{
	switch (ulEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:

		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:
		autoTexting();
		break;
	default:
		break;
	}
}
/* 自動送り */
void CMainWindow::autoTexting()
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		if (!m_pSceneCrafter->hasReachedLastScene())
		{
			shiftText(true);
		}
	}
}
