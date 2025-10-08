
#include <Windows.h>
#include <CommCtrl.h>

#include "main_window.h"

#include "win_filesystem.h"
#include "win_dialogue.h"
#include "native-ui/media_setting_dialogue.h"
#include "text_utility.h"

CMainWindow::CMainWindow()
{

}

CMainWindow::~CMainWindow()
{

}

bool CMainWindow::Create(HINSTANCE hInstance, const wchar_t* pwzWindowName)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	//wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_APP));
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
	//wcex.lpszMenuName = MAKEINTRESOURCE(IDI_ICON_APP);
	wcex.lpszClassName = m_swzClassName;
	//wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON_APP));

	if (::RegisterClassExW(&wcex))
	{
		m_hInstance = hInstance;

		UINT uiDpi = ::GetDpiForSystem();
		int iWindowWidth = ::MulDiv(200, uiDpi, USER_DEFAULT_SCREEN_DPI);
		int iWindowHeight = ::MulDiv(200, uiDpi, USER_DEFAULT_SCREEN_DPI);

		m_hWnd = ::CreateWindowW(m_swzClassName, pwzWindowName, WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, CW_USEDEFAULT, iWindowWidth, iWindowHeight, nullptr, nullptr, hInstance, this);
		if (m_hWnd != nullptr)
		{
			return true;
		}
		else
		{
			std::wstring wstrMessage = L"CreateWindowExW failed; code: " + std::to_wstring(::GetLastError());
			::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
		}
	}
	else
	{
		std::wstring wstrMessage = L"RegisterClassW failed; code: " + std::to_wstring(::GetLastError());
		::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
	}

	return false;
}

int CMainWindow::MessageLoop()
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
			/*ループ終了*/
			return static_cast<int>(msg.wParam);
		}
		else
		{
			/*ループ異常*/
			std::wstring wstrMessage = L"GetMessageW failed; code: " + std::to_wstring(::GetLastError());
			::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
			return -1;
		}
	}
	return 0;
}
/*C CALLBACK*/
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
		return pThis->HandleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*メッセージ処理*/
LRESULT CMainWindow::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate(hWnd);
	case WM_DESTROY:
		return OnDestroy();
	case WM_CLOSE:
		return OnClose();
	case WM_PAINT:
		return OnPaint();
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		return OnKeyDown(wParam, lParam);
	case WM_KEYUP:
		return OnKeyUp(wParam, lParam);
	case WM_COMMAND:
		return OnCommand(wParam, lParam);
	case WM_TIMER:
		return OnTimer(wParam);
	case WM_MOUSEMOVE:
		return OnMouseMove(wParam, lParam);
	case WM_MOUSEWHEEL:
		return OnMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return OnLButtonDown(wParam, lParam);
	case WM_LBUTTONUP:
		return OnLButtonUp(wParam, lParam);
	case WM_RBUTTONUP:
		return OnRButtonUp(wParam, lParam);
	case WM_MBUTTONUP:
		return OnMButtonUp(wParam, lParam);
	case EventMessage::kAudioPlayer:
		OnAudioPlayerEvent(static_cast<unsigned long>(lParam));
		break;
	default:
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/*WM_CREATE*/
LRESULT CMainWindow::OnCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	InitialiseMenuBar();

	m_pD2ImageDrawer = std::make_unique<CD2ImageDrawer>(m_hWnd);

	m_pD2TextWriter = std::make_unique<CD2TextWriter>(m_pD2ImageDrawer->GetD2Factory(), m_pD2ImageDrawer->GetD2DeviceContext());
	m_pD2TextWriter->SetupOutLinedDrawing(L"C:\\Windows\\Fonts\\yumindb.ttf");

	m_pViewManager = std::make_unique<CViewManager>(m_hWnd);

	m_pMfVoicePlayer = std::make_unique<CMfMediaPlayer>();
	m_pMfVoicePlayer->SetPlaybackWindow(m_hWnd, EventMessage::kAudioPlayer);

	m_pSceneCrafter = std::make_unique<CLilyanSceneCrafter>(m_pD2ImageDrawer->GetD2DeviceContext());

	m_pFontSettingDialogue = std::make_unique<CFontSettingDialogue>();

	return 0;
}
/*WM_DESTROY*/
LRESULT CMainWindow::OnDestroy()
{
	::PostQuitMessage(0);

	return 0;
}
/*WM_CLOSE*/
LRESULT CMainWindow::OnClose()
{
	::KillTimer(m_hWnd, Timer::kText);

	::DestroyWindow(m_hWnd);
	::UnregisterClassW(m_swzClassName, m_hInstance);

	return 0;
}
/*WM_PAINT*/
LRESULT CMainWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	if (m_pD2ImageDrawer == nullptr || m_pD2TextWriter == nullptr || m_pViewManager == nullptr || m_pSceneCrafter == nullptr)
	{
		::EndPaint(m_hWnd, &ps);
		return 0;
	}

	m_pD2ImageDrawer->Clear();

	ID2D1Bitmap* pImage = m_pSceneCrafter->GetCurrentImage();
	if (pImage != nullptr)
	{
		m_pD2ImageDrawer->Draw(pImage, { m_pViewManager->GetXOffset(), m_pViewManager->GetYOffset() }, m_pViewManager->GetScale());

		if (!m_isTextHidden)
		{
			std::wstring wstr = m_pSceneCrafter->GetCurrentFormattedText();
			m_pD2TextWriter->OutLinedDraw(wstr.c_str(), static_cast<unsigned long>(wstr.size()));
		}
		m_pD2ImageDrawer->Display();
	}

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/*WM_SIZE*/
LRESULT CMainWindow::OnSize()
{
	return 0;
}
/*WM_KEYDOWN*/
LRESULT CMainWindow::OnKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		AutoTexting();
		break;
	case VK_LEFT:
		ShiftText(false);
		break;
	default:

		break;
	}

	return 0;
}
/*WM_KEYUP*/
LRESULT CMainWindow::OnKeyUp(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_ESCAPE:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	case VK_UP:
		MenuOnForeFile();
		break;
	case VK_DOWN:
		MenuOnNextFile();
		break;
	case 'C':
		if (m_pD2TextWriter.get() != nullptr)
		{
			m_pD2TextWriter->SwitchTextColour();
			UpdateScreen();
		}
		break;
	case 'T':
		m_isTextHidden ^= true;
		UpdateScreen();
		break;
	}
	return 0;
}
/*WM_COMMAND*/
LRESULT CMainWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmKind = LOWORD(lParam);
	if (wmKind == 0)
	{
		/*Menus*/
		switch (wmId)
		{
		case Menu::kOpenFile:
			MenuOnOpenFile();
			break;
		case Menu::kVoiceSetting:
			MenuOnVoiceSetting();
			break;
		case Menu::kSoundSetting:
			MenuOnSoundSetting();
			break;
		case Menu::kFontSetting:
			MenuOnFontSetting();
			break;
		default:
			break;
		}
	}
	else
	{
		/*Controls*/
	}

	return 0;
}
/*WM_TIMER*/
LRESULT CMainWindow::OnTimer(WPARAM wParam)
{
	switch (wParam)
	{
	case Timer::kText:
		if (m_pMfVoicePlayer != nullptr)
		{
			if (m_pMfVoicePlayer->IsEnded())
			{
				AutoTexting();
			}
		}
		break;
	default:
		break;
	}
	return 0;
}
/* WM_MOUSEMOVE */
LRESULT CMainWindow::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);
	if (usKey == MK_LBUTTON)
	{
		if (m_wasLeftCombinated)return 0;

		POINT pt{};
		::GetCursorPos(&pt);
		int iX = m_lastCursorPos.x - pt.x;
		int iY = m_lastCursorPos.y - pt.y;

		if (m_hasLeftBeenDragged)
		{
			if (m_pViewManager != nullptr)
			{
				m_pViewManager->SetOffset(iX, iY);
				UpdateScreen();
			}
		}

		m_lastCursorPos = pt;
		m_hasLeftBeenDragged = true;
	}

	return 0;
}
/*WM_MOUSEWHEEL*/
LRESULT CMainWindow::OnMouseWheel(WPARAM wParam, LPARAM lParam)
{
	int iScroll = -static_cast<short>(HIWORD(wParam)) / WHEEL_DELTA;
	WORD wKey = LOWORD(wParam);

	if (wKey == 0)
	{
		if (m_pViewManager.get() != nullptr)
		{
			m_pViewManager->Rescale(iScroll > 0);
		}
	}
	else if (wKey == MK_LBUTTON)
	{

	}
	else if (wKey == MK_RBUTTON)
	{
		ShiftText(iScroll > 0);

		m_wasRightCombinated = true;
	}

	return 0;
}
/*WM_LBUTTONDOWN*/
LRESULT CMainWindow::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_lastCursorPos);

	m_wasLeftPressed = true;

	return 0;
}
/*WM_LBUTTONUP*/
LRESULT CMainWindow::OnLButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);

	if (usKey == MK_RBUTTON && m_isBarHidden)
	{
		::PostMessage(m_hWnd, WM_SYSCOMMAND, SC_MOVE, 0);
		INPUT input{};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = VK_DOWN;
		::SendInput(1, &input, sizeof(input));
	}

	if (usKey == 0 && m_wasLeftPressed)
	{
		POINT pt{};
		::GetCursorPos(&pt);
		int iX = m_lastCursorPos.x - pt.x;
		int iY = m_lastCursorPos.y - pt.y;

		if (iX == 0 && iY == 0)
		{

		}
	}

	m_wasLeftPressed = false;

	return 0;
}
/*WM_RBUTTONUP*/
LRESULT CMainWindow::OnRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_wasRightCombinated)
	{
		m_wasRightCombinated = false;
		return 0;
	}

	return 0;
}
/*WM_MBUTTONUP*/
LRESULT CMainWindow::OnMButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD usKey = LOWORD(wParam);

	if (usKey == 0)
	{
		if (m_pViewManager.get() != nullptr)
		{
			m_pViewManager->ResetZoom();
		}
	}

	if (usKey == MK_RBUTTON)
	{
		ToggleWindowBorderStyle();
	}

	return 0;
}
/*操作欄作成*/
void CMainWindow::InitialiseMenuBar()
{
	HMENU hMenuFile = nullptr;
	HMENU hMenuSetting = nullptr;
	HMENU hMenuBar = nullptr;
	BOOL iRet = FALSE;

	if (m_hMenuBar != nullptr)return;

	hMenuFile = ::CreateMenu();
	if (hMenuFile == nullptr)goto failed;

	iRet = ::AppendMenuA(hMenuFile, MF_STRING, Menu::kOpenFile, "Open");
	if (iRet == 0)goto failed;

	hMenuSetting = ::CreateMenu();
	if (hMenuSetting == nullptr)goto failed;

	iRet = ::AppendMenuA(hMenuSetting, MF_STRING, Menu::kVoiceSetting, "Voice");
	if (iRet == 0)goto failed;
	iRet = ::AppendMenuA(hMenuSetting, MF_STRING, Menu::kSoundSetting, "Sound");
	if (iRet == 0)goto failed;
	iRet = ::AppendMenuA(hMenuSetting, MF_STRING, Menu::kFontSetting, "Font");
	if (iRet == 0)goto failed;

	hMenuBar = ::CreateMenu();
	if (hMenuBar == nullptr) goto failed;

	iRet = ::AppendMenuA(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuFile), "File");
	if (iRet == 0)goto failed;
	iRet = ::AppendMenuA(hMenuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(hMenuSetting), "Setting");
	if (iRet == 0)goto failed;

	iRet = ::SetMenu(m_hWnd, hMenuBar);
	if (iRet == 0)goto failed;

	m_hMenuBar = hMenuBar;

	return;

failed:
	std::wstring wstrMessage = L"Failed to create menu; code: " + std::to_wstring(::GetLastError());
	::MessageBoxW(nullptr, wstrMessage.c_str(), L"Error", MB_ICONERROR);
	if (hMenuFile != nullptr)
	{
		::DestroyMenu(hMenuFile);
	}
	if (hMenuSetting != nullptr)
	{
		::DestroyMenu(hMenuSetting);
	}
	if (hMenuBar != nullptr)
	{
		::DestroyMenu(hMenuBar);
	}
}
/*ファイル選択*/
void CMainWindow::MenuOnOpenFile()
{
	const wchar_t swzFilter[] = L"chara*_20*.nani";
	std::wstring wstrSelectedFilePath = win_dialogue::SelectOpenFile(L"Script file", swzFilter, nullptr, m_hWnd);
	if (!wstrSelectedFilePath.empty())
	{
		bool bRet = SetupScenario(wstrSelectedFilePath.c_str());
		if (bRet)
		{
			m_scriptFilePaths.clear();
			m_nScriptFileIndex = 0;
			win_filesystem::GetFilePathListAndIndex(wstrSelectedFilePath.c_str(), swzFilter, m_scriptFilePaths, &m_nScriptFileIndex);
		}
	}
}
/*次のファイルに移動*/
void CMainWindow::MenuOnNextFile()
{
	if (m_scriptFilePaths.empty())return;

	++m_nScriptFileIndex;
	if (m_nScriptFileIndex >= m_scriptFilePaths.size())m_nScriptFileIndex = 0;

	SetupScenario(m_scriptFilePaths[m_nScriptFileIndex].c_str());
}
/*前のファイルに移動*/
void CMainWindow::MenuOnForeFile()
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptFileIndex;
	if (m_nScriptFileIndex >= m_scriptFilePaths.size())m_nScriptFileIndex = m_scriptFilePaths.size() - 1;

	SetupScenario(m_scriptFilePaths[m_nScriptFileIndex].c_str());
}
/*音声音量・再生速度変更*/
void CMainWindow::MenuOnVoiceSetting()
{
	if (m_pMfVoicePlayer.get() != nullptr)
	{
		CMediaSettingDialogue sMediaSettingDialogue;
		sMediaSettingDialogue.Open(m_hInstance, m_hWnd, m_pMfVoicePlayer.get(), L"Voice");
	}
}
/*効果音音量・再生速度変更*/
void CMainWindow::MenuOnSoundSetting()
{
	if (m_pMfSoundPlayer.get() != nullptr)
	{
		CMediaSettingDialogue sMediaSettingDialogue;
		sMediaSettingDialogue.Open(m_hInstance, m_hWnd, m_pMfSoundPlayer.get(), L"Sound");
	}
}

void CMainWindow::MenuOnFontSetting()
{
	if (m_pFontSettingDialogue != nullptr)
	{
		if (m_pFontSettingDialogue->GetHwnd() == nullptr)
		{
			HWND hWnd = m_pFontSettingDialogue->Open(m_hInstance, m_hWnd, L"Font", m_pD2TextWriter.get());
			::ShowWindow(hWnd, SW_SHOWNORMAL);
		}
		else
		{
			::SetFocus(m_pFontSettingDialogue->GetHwnd());
		}
	}
}
/*表示形式切り替え*/
void CMainWindow::ToggleWindowBorderStyle()
{
	RECT rect;
	::GetWindowRect(m_hWnd, &rect);
	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	m_isBarHidden ^= true;

	if (m_isBarHidden)
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle & ~WS_CAPTION & ~WS_SYSMENU);
		::SetWindowPos(m_hWnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		::SetMenu(m_hWnd, nullptr);
	}
	else
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle | WS_CAPTION | WS_SYSMENU);
		::SetMenu(m_hWnd, m_hMenuBar);
	}

	if (m_pViewManager.get() != nullptr)
	{
		m_pViewManager->OnStyleChanged();
	}
}
/*寸劇構築*/
bool CMainWindow::SetupScenario(const wchar_t* pwzFilePath)
{
	bool bRet = false;

	if (m_pSceneCrafter.get() != nullptr)
	{
		bRet = m_pSceneCrafter->LoadScenario(pwzFilePath);
		if (bRet)
		{
			unsigned int uiWidth = 0;
			unsigned int uiHeight = 0;
			m_pSceneCrafter->GetImageSize(&uiWidth, &uiHeight);

			if (m_pViewManager != nullptr)
			{
				m_pViewManager->SetBaseSize(uiWidth, uiHeight);
				m_pViewManager->ResetZoom();
			}

			std::wstring wsrWindowTitle = m_pSceneCrafter->GetSceneTitle();
			wsrWindowTitle += L"; " + text_utility::ExtractFileName(pwzFilePath);
			::SetWindowText(m_hWnd, wsrWindowTitle.c_str());

			m_pMfSoundPlayer = std::make_unique<CMfMediaPlayer>();

			UpdateText();
		}
		else
		{
			std::wstring wstrMessage = L"Failed to set up ";
			wstrMessage += pwzFilePath;
			::MessageBox(m_hWnd, wstrMessage.c_str(), L"Error", MB_ICONERROR);
		}
	}

	return bRet;
}
/*再描画要求*/
void CMainWindow::UpdateScreen()
{
	::InvalidateRect(m_hWnd, nullptr, FALSE);
}
/*文章送り・戻し*/
void CMainWindow::ShiftText(bool bForward)
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		m_pSceneCrafter->ShiftScene(bForward);
		UpdateText();
	}
}
/*文章更新*/
void CMainWindow::UpdateText()
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		if (m_pMfSoundPlayer.get() != nullptr)
		{
			const wchar_t* pwzSoundFilePath = m_pSceneCrafter->GetCurrentSoundFilePath();
			if (pwzSoundFilePath != nullptr && *pwzSoundFilePath != L'\0')
			{
				m_pMfSoundPlayer->Play(pwzSoundFilePath);
			}
		}

		if (m_pMfVoicePlayer.get() != nullptr)
		{
			const wchar_t *pwzVoiceFilePath = m_pSceneCrafter->GetCurrentVoiceFilePath();
			if (pwzVoiceFilePath != nullptr && *pwzVoiceFilePath != L'\0')
			{
				m_pMfVoicePlayer->Play(pwzVoiceFilePath);
			}
		}

		constexpr unsigned int kTimerInterval = 2000;
		::SetTimer(m_hWnd, Timer::kText, kTimerInterval, nullptr);

		UpdateScreen();
	}
}
/*IMFMediaEngineNotify::EventNotify*/
void CMainWindow::OnAudioPlayerEvent(unsigned long ulEvent)
{
	switch (ulEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:

		break;
	case MF_MEDIA_ENGINE_EVENT_ENDED:
		AutoTexting();
		break;
	default:
		break;
	}
}
/*自動送り*/
void CMainWindow::AutoTexting()
{
	if (m_pSceneCrafter.get() != nullptr)
	{
		if (!m_pSceneCrafter->HasReachedLastScene())
		{
			ShiftText(true);
		}
	}
}
