#ifndef LILY_SCENE_CRAFTER_H_
#define LILY_SCENE_CRAFTER_H_

#include <Windows.h>
#include <d2d1_1.h>
#include <atlbase.h>

#include <string>
#include <vector>

#include "adv.h"

class CLilyanSceneCrafter
{
public:
	CLilyanSceneCrafter(ID2D1DeviceContext* pD2d1DeviceContext);
	~CLilyanSceneCrafter();

	bool LoadScenario(const wchar_t* pwzScenarioFilePath);
	void GetImageSize(unsigned int* uiWidth, unsigned int* uiHeight);
	std::wstring& GetSceneTitle();

	void ShiftScene(bool bForward);
	bool HasReachedLastScene();

	ID2D1Bitmap* GetCurrentImage();
	std::wstring GetCurrentFormattedText();
	const wchar_t* GetCurrentVoiceFilePath();
	const wchar_t* GetCurrentSoundFilePath();
private:
	ID2D1DeviceContext* m_pStoredD2d1DeviceContext = nullptr;

	std::wstring m_wstrSceneTitle;

	std::vector<adv::TextDatum> m_textData;

	std::vector<adv::SceneDatum> m_sceneData;
	size_t m_nSceneIndex = 0;

	std::vector<CComPtr<ID2D1Bitmap>> m_images;

	std::vector<adv::SoundDatum> m_soundData;

	void ClearScenarioData();
	ID2D1Bitmap* ImportWholeImage(const std::wstring& wstrImageFilePath);
};

#endif // !LILY_SCENE_CRAFTER_H_
