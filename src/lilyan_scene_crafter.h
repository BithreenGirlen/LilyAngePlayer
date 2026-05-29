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

	bool loadScenario(const std::wstring& scenarioFilePath);
	bool hasScenarioData() const;

	void getCurrentImageSize(unsigned int* width, unsigned int* height);
	void getLargestImageSize(unsigned int* width, unsigned int* height);
	const std::wstring& getSceneTitle() const noexcept;

	void syncImage(bool synchronised);
	bool isImageSynced() const;
	void shiftForwardImage();

	void shiftScene(bool forward);
	bool hasReachedLastScene();

	ID2D1Bitmap* getCurrentImage();
	const std::wstring& getCurrentFormattedText() const noexcept;
	const wchar_t* getCurrentVoiceFilePath();
	const wchar_t* getCurrentSoundFilePath();

	const std::vector<adv::LabelDatum>& getLabelData() const noexcept;
	bool jumpToLabel(size_t nLabelIndex);
private:
	ID2D1DeviceContext* m_pStoredD2d1DeviceContext = nullptr;

	std::wstring m_sceneTitle;
	std::vector<adv::TextDatum> m_textData;
	std::vector<adv::SceneDatum> m_sceneData;
	size_t m_nSceneIndex = 0;
	std::vector<adv::SoundDatum> m_soundData;
	std::vector<adv::LabelDatum> m_labelData;

	std::vector<CComPtr<ID2D1Bitmap>> m_images;
	size_t m_nImageIndex = 0;
	bool m_isImageSynced = true;

	std::wstring m_formattedText;

	void clearScenarioData();
	ID2D1Bitmap* importWholeImage(const std::wstring& imageFilePath);

	void prepareScene();
	void prepareText();
};

#endif // !LILY_SCENE_CRAFTER_H_
