#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>

#include <string>
#include <vector>
#include <memory>

#include "d2_image_drawer.h"
#include "d2_text_writer.h"
#include "view_manager.h"
#include "mf_media_player.h"
#include "lilyan_scene_crafter.h"
#include "native-ui/font_setting_dialogue.h"

class CMainWindow
{
public:
	CMainWindow();
	~CMainWindow();

	bool create(HINSTANCE hInstance, const wchar_t* pwzWindowName);
	int messageLoop();

	HWND getHwnd()const { return m_hWnd;}
private:
	const wchar_t* m_className = L"Mijn player window";

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onCreate(HWND hWnd);
	LRESULT onDestroy();
	LRESULT onClose();
	LRESULT onPaint();
	LRESULT onSize();
	LRESULT onKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT onKeyUp(WPARAM wParam, LPARAM lParam);
	LRESULT onCommand(WPARAM wParam, LPARAM lParam);
	LRESULT onTimer(WPARAM wParam);
	LRESULT onMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT onMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onRButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onMButtonUp(WPARAM wParam, LPARAM lParam);

	struct Menu
	{
		enum
		{
			kOpenFile = 1,
			kVoiceSetting, kSoundSetting, kFontSetting,
			kSyncImage,
			kLabelStartIndex
		};
	};
	struct MenuBar { enum { kFile, kSetting, kImage }; };
	struct EventMessage { enum { kAudioPlayer = WM_USER + 1 }; };
	struct Timer { enum { kText = 1, }; };

	struct MouseState
	{
		bool wasLeftPressed = false;
		bool hasLeftBeenDragged = false;
		bool wasLeftCombined = false;
		bool wasRightCombined = false;
		/// @brief Last mouse position in client coördinate
		POINT lastMousePos{};
	};

	MouseState m_mouseState;

	HMENU m_hMenuBar = nullptr;
	bool m_isBarHidden = false;

	bool m_isTextHidden = false;

	std::vector<std::wstring> m_scriptFilePaths;
	size_t m_nScriptFileIndex = 0;

	void initialiseMenuBar();
	void showErrorMessageBox(const wchar_t* format, ...) const;

	void menuOnOpenFile();
	void menuOnNextFile();
	void menuOnForeFile();

	void menuOnVoiceSetting();
	void menuOnSoundSetting();
	void menuOnFontSetting();
	void menuOnSyncImage();

	void toggleWindowBorderStyle();
	bool setMenuCheckState(unsigned int uiMenuIndex, unsigned int uiItemIndex, bool checked) const;

	bool setupScenario(const std::wstring& filePath);
	void jumpScene(size_t nIndex);

	void updateScreen();

	std::unique_ptr<CD2ImageDrawer> m_pD2ImageDrawer;
	std::unique_ptr<CD2TextWriter> m_pD2TextWriter;
	std::unique_ptr<CViewManager> m_pViewManager;
	std::unique_ptr<CMfMediaPlayer> m_pMfVoicePlayer;
	std::unique_ptr<CMfMediaPlayer> m_pMfSoundPlayer;
	std::unique_ptr<CLilyanSceneCrafter> m_pSceneCrafter;
	std::unique_ptr<CFontSettingDialogue> m_pFontSettingDialogue;

	void shiftText(bool bForward);
	void updateText();
	void onAudioPlayerEvent(unsigned long ulEvent);
	void autoTexting();
};

#endif //MAIN_WINDOW_H_