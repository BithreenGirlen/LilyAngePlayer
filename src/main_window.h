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

class CMainWindow
{
public:
	CMainWindow();
	~CMainWindow();

	bool Create(HINSTANCE hInstance, const wchar_t* pwzWindowName);
	int MessageLoop();

	HWND GetHwnd()const { return m_hWnd;}
private:
	const wchar_t* m_swzClassName = L"Mijn player window";
	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCreate(HWND hWnd);
	LRESULT OnDestroy();
	LRESULT OnClose();
	LRESULT OnPaint();
	LRESULT OnSize();
	LRESULT OnKeyUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnTimer(WPARAM wParam);
	LRESULT OnMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT OnLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT OnMButtonUp(WPARAM wParam, LPARAM lParam);

	enum Menu
	{
		kOpenFile = 1,
		kVoiceSetting, kSoundSetting
	};
	enum MenuBar
	{
		kFile, kAudio
	};
	enum EventMessage
	{
		kAudioPlayer = WM_USER + 1
	};
	enum Timer
	{
		kText = 1,
	};

	POINT m_CursorPos{};
	bool m_bLeftDowned = false;

	HMENU m_hMenuBar = nullptr;
	bool m_bBarHidden = false;

	bool m_bTextHidden = false;

	std::vector<std::wstring> m_scriptFilePaths;
	size_t m_nScriptFileIndex = 0;

	void InitialiseMenuBar();

	void MenuOnOpenFile();
	void MenuOnNextFile();
	void MenuOnForeFile();

	void MenuOnVoiceSetting();
	void MenuOnSoundSetting();

	void SwitchWindowStyle();

	bool SetupScenario(const wchar_t* pwzFilePath);

	void UpdateScreen();

	std::unique_ptr<CD2ImageDrawer> m_pD2ImageDrawer;
	std::unique_ptr<CD2TextWriter> m_pD2TextWriter;
	std::unique_ptr<CViewManager> m_pViewManager;
	std::unique_ptr<CMfMediaPlayer> m_pMfVoicePlayer;
	std::unique_ptr<CMfMediaPlayer> m_pMfSoundPlayer;
	std::unique_ptr<CLilyanSceneCrafter> m_pSceneCrafter;

	void ShiftText(bool bForward);
	void UpdateText();
	void OnAudioPlayerEvent(unsigned long ulEvent);
	void AutoTexting();
};

#endif //MAIN_WINDOW_H_