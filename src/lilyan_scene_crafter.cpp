

#include "lilyan_scene_crafter.h"

#include "lilyan.h"
#include "win_image.h"

CLilyanSceneCrafter::CLilyanSceneCrafter(ID2D1DeviceContext* pD2d1DeviceContext)
	: m_pStoredD2d1DeviceContext(pD2d1DeviceContext)
{

}

CLilyanSceneCrafter::~CLilyanSceneCrafter()
{

}

bool CLilyanSceneCrafter::LoadScenario(const wchar_t* pwzScenarioFilePath)
{
	if (pwzScenarioFilePath == nullptr)return false;
	ClearScenarioData();

	std::vector<std::wstring> wstrImageFilePaths;
	lilyan::LoadScenario(pwzScenarioFilePath, m_textData, wstrImageFilePaths, m_sceneData, m_wstrSceneTitle, m_soundData);
	if (!m_wstrSceneTitle.empty())
	{
		m_wstrSceneTitle.erase(0, 1);
	}

	for (const auto& wstrImageFilePath : wstrImageFilePaths)
	{
		ImportWholeImage(wstrImageFilePath);
	}

	return !m_images.empty();
}
/*画像寸法取得*/
void CLilyanSceneCrafter::GetImageSize(unsigned int* uiWidth, unsigned int* uiHeight)
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nImageIndex = m_sceneData[m_nSceneIndex].nImageIndex;
		if (nImageIndex < m_images.size())
		{
			D2D1_SIZE_U s = m_images[nImageIndex]->GetPixelSize();
			*uiWidth = s.width;
			*uiHeight = s.height;
		}
	}
}
/*題名受け渡し*/
std::wstring& CLilyanSceneCrafter::GetSceneTitle()
{
	return m_wstrSceneTitle;
}
/*場面移行*/
void CLilyanSceneCrafter::ShiftScene(bool bForward)
{
	if (m_sceneData.empty())return;

	if (bForward)
	{
		if (++m_nSceneIndex >= m_sceneData.size())
		{
			m_nSceneIndex = 0;
		}
	}
	else
	{
		if (--m_nSceneIndex >= m_sceneData.size())
		{
			m_nSceneIndex = m_sceneData.size() - 1;
		}
	}
}
/*最終場面是否*/
bool CLilyanSceneCrafter::HasReachedLastScene()
{
	return m_nSceneIndex == m_sceneData.size() - 1;
}
/*現在の画像受け渡し*/
ID2D1Bitmap* CLilyanSceneCrafter::GetCurrentImage()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nImageIndex = m_sceneData[m_nSceneIndex].nImageIndex;
		if (nImageIndex < m_images.size())
		{
			return m_images[nImageIndex];
		}
	}

	return nullptr;
}
/*文章生成*/
std::wstring CLilyanSceneCrafter::GetCurrentFormattedText()
{
	std::wstring wstr;
	if (m_nSceneIndex < m_sceneData.size())
	{
		wstr.reserve(128);
		size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;

		if (nTextIndex < m_textData.size())
		{
			wstr = m_textData[nTextIndex].wstrText;
			if (!wstr.empty() && wstr.back() != L'\n')wstr.push_back(L'\n');
			wstr += std::to_wstring(nTextIndex + 1) + L"/" + std::to_wstring(m_textData.size());
		}
	}

	return wstr;
}
/*現在の音声ファイル経路受け渡し*/
const wchar_t* CLilyanSceneCrafter::GetCurrentVoiceFilePath()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;
		if (nTextIndex < m_textData.size())
		{
			return m_textData[nTextIndex].wstrVoicePath.c_str();
		}
	}

	return nullptr;
}
/*現在の効果音ファイル経路受け渡し*/
const wchar_t* CLilyanSceneCrafter::GetCurrentSoundFilePath()
{
	const auto iter = std::find_if
	(
		m_soundData.begin(), m_soundData.end(),
		[this](adv::SoundDatum& soundDatum)
		{
			return soundDatum.nSceneIndex == m_nSceneIndex;
		}
	);

	if (iter != m_soundData.cend())
	{
		return m_soundData[std::distance(m_soundData.begin(), iter)].wstrSoundFilePath.c_str();
	}

	return nullptr;
}
/*消去*/
void CLilyanSceneCrafter::ClearScenarioData()
{
	m_textData.clear();

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_images.clear();

	m_wstrSceneTitle.clear();

	m_soundData.clear();
}

ID2D1Bitmap* CLilyanSceneCrafter::ImportWholeImage(const std::wstring& wstrImageFilePath)
{
	ID2D1Bitmap* p = nullptr;

	SImageFrame sImageFrame{};
	bool bRet = win_image::LoadImageToMemory(wstrImageFilePath.c_str(), &sImageFrame);
	if (bRet)
	{
		CComPtr<ID2D1Bitmap> pD2d1Bitmap;

		HRESULT hr = m_pStoredD2d1DeviceContext->CreateBitmap(
			D2D1::SizeU(sImageFrame.uiWidth, sImageFrame.uiHeight),
			D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&pD2d1Bitmap);

		D2D1_RECT_U rc = { 0, 0, sImageFrame.uiWidth, sImageFrame.uiHeight };
		hr = pD2d1Bitmap->CopyFromMemory(&rc, sImageFrame.pixels.data(), sImageFrame.iStride);
		if (SUCCEEDED(hr))
		{
			m_images.push_back(std::move(pD2d1Bitmap));
			p = m_images.back().p;
		}
	}

	return p;
}
