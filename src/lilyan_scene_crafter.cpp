

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
	lilyan::LoadScenario(pwzScenarioFilePath, m_textData, wstrImageFilePaths, m_sceneData, m_wstrSceneTitle, m_soundData, m_labelData);
	if (!m_wstrSceneTitle.empty() && m_wstrSceneTitle[0] == L';')
	{
		m_wstrSceneTitle.erase(0, 1);
	}

	for (const auto& wstrImageFilePath : wstrImageFilePaths)
	{
		ImportWholeImage(wstrImageFilePath);
	}

	return !m_images.empty();
}

bool CLilyanSceneCrafter::HasScenarioData() const
{
	return !m_sceneData.empty();
}
/*画像寸法取得*/
void CLilyanSceneCrafter::GetCurrentImageSize(unsigned int* uiWidth, unsigned int* uiHeight)
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

void CLilyanSceneCrafter::GetLargestImageSize(unsigned int* uiWidth, unsigned int* uiHeight)
{
	unsigned int uiMaxWidth = 0;
	unsigned int uiMaxHeight = 0;

	for (const auto& pD2Bitmap : m_images)
	{
		D2D1_SIZE_U s = pD2Bitmap->GetPixelSize();

		uiMaxWidth = (std::max)(uiMaxWidth, s.width);
		uiMaxHeight = (std::max)(uiMaxHeight, s.height);;
	}

	if (uiWidth != nullptr)*uiWidth = uiMaxWidth;
	if (uiHeight != nullptr)*uiHeight = uiMaxHeight;
}
/*題名受け渡し*/
std::wstring& CLilyanSceneCrafter::GetSceneTitle()
{
	return m_wstrSceneTitle;
}

void CLilyanSceneCrafter::ToggleImageSync()
{
	m_isImageSynced ^= true;
}

bool CLilyanSceneCrafter::IsImageSynced() const
{
	return m_isImageSynced;
}

void CLilyanSceneCrafter::ShiftImage()
{
	if (!m_isImageSynced)
	{
		++m_nImageIndex;
		if (m_nImageIndex >= m_images.size())m_nImageIndex = 0;
	}
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
		if (m_isImageSynced)
		{
			m_nImageIndex = m_sceneData[m_nSceneIndex].nImageIndex;
		}

		if (m_nImageIndex < m_images.size())
		{
			return m_images[m_nImageIndex];
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

const std::vector<adv::LabelDatum>& CLilyanSceneCrafter::GetLabelData() const
{
	return m_labelData;
}

bool CLilyanSceneCrafter::JumpToLabel(size_t nLabelIndex)
{
	if (nLabelIndex < m_labelData.size())
	{
		const auto& labelDatum = m_labelData[nLabelIndex];

		if (labelDatum.nSceneIndex < m_sceneData.size())
		{
			m_nSceneIndex = labelDatum.nSceneIndex;

			return true;
		}
	}
	return false;
}
/*消去*/
void CLilyanSceneCrafter::ClearScenarioData()
{
	m_textData.clear();

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_images.clear();
	m_nImageIndex = 0;

	m_wstrSceneTitle.clear();

	m_soundData.clear();

	m_labelData.clear();
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
