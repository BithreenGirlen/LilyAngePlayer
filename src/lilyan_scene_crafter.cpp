

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

bool CLilyanSceneCrafter::loadScenario(const std::wstring& scenarioFilePath)
{
	clearScenarioData();

	std::vector<std::wstring> imageFilePaths;
	lilyan::LoadScenario(scenarioFilePath, m_textData, imageFilePaths, m_sceneData, m_sceneTitle, m_soundData, m_labelData);
	if (!m_sceneTitle.empty() && m_sceneTitle[0] == L';')
	{
		m_sceneTitle.erase(0, 1);
	}

	for (const auto& imageFilePath : imageFilePaths)
	{
		importWholeImage(imageFilePath);
	}

	return !m_images.empty();
}

bool CLilyanSceneCrafter::hasScenarioData() const
{
	return !m_sceneData.empty();
}
/* 画像寸法取得 */
void CLilyanSceneCrafter::getCurrentImageSize(unsigned int* width, unsigned int* height)
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nImageIndex = m_sceneData[m_nSceneIndex].nImageIndex;
		if (nImageIndex < m_images.size())
		{
			D2D1_SIZE_U s = m_images[nImageIndex]->GetPixelSize();
			*width = s.width;
			*height = s.height;
		}
	}
}

void CLilyanSceneCrafter::getLargestImageSize(unsigned int* width, unsigned int* height)
{
	unsigned int largestWidth = 0;
	unsigned int largestHeight = 0;

	for (const auto& pD2Bitmap : m_images)
	{
		D2D1_SIZE_U s = pD2Bitmap->GetPixelSize();

		largestWidth = (std::max)(largestWidth, s.width);
		largestHeight = (std::max)(largestHeight, s.height);;
	}

	if (width != nullptr)*width = largestWidth;
	if (height != nullptr)*height = largestHeight;
}
/* 題名受け渡し */
const std::wstring& CLilyanSceneCrafter::getSceneTitle() const noexcept
{
	return m_sceneTitle;
}

void CLilyanSceneCrafter::syncImage(bool synchronised)
{
	m_isImageSynced = synchronised;
}

bool CLilyanSceneCrafter::isImageSynced() const
{
	return m_isImageSynced;
}

void CLilyanSceneCrafter::shiftForwardImage()
{
	if (!m_isImageSynced)
	{
		++m_nImageIndex;
		if (m_nImageIndex >= m_images.size())m_nImageIndex = 0;
	}
}
/* 場面移行 */
void CLilyanSceneCrafter::shiftScene(bool forward)
{
	if (m_sceneData.empty())return;

	if (forward)
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
/* 最終場面是否 */
bool CLilyanSceneCrafter::hasReachedLastScene()
{
	return m_nSceneIndex == m_sceneData.size() - 1;
}
/* 現在の画像受け渡し */
ID2D1Bitmap* CLilyanSceneCrafter::getCurrentImage()
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
/* 文章生成 */
std::wstring CLilyanSceneCrafter::getCurrentFormattedText()
{
	std::wstring wstr;
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;
		if (nTextIndex < m_textData.size())
		{
			wstr = m_textData[nTextIndex].message;
			if (!wstr.empty() && wstr.back() != L'\n')wstr.push_back(L'\n');
			wstr += std::to_wstring(nTextIndex + 1) + L"/" + std::to_wstring(m_textData.size());
		}
	}

	return wstr;
}
/* 現在の音声ファイル経路受け渡し */
const wchar_t* CLilyanSceneCrafter::getCurrentVoiceFilePath()
{
	if (m_nSceneIndex < m_sceneData.size())
	{
		size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;
		if (nTextIndex < m_textData.size())
		{
			return m_textData[nTextIndex].voiceFilePath.c_str();
		}
	}

	return nullptr;
}
/* 現在の効果音ファイル経路受け渡し */
const wchar_t* CLilyanSceneCrafter::getCurrentSoundFilePath()
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
		return m_soundData[std::distance(m_soundData.begin(), iter)].soundFilePath.c_str();
	}

	return nullptr;
}

const std::vector<adv::LabelDatum>& CLilyanSceneCrafter::getLabelData() const noexcept
{
	return m_labelData;
}

bool CLilyanSceneCrafter::jumpToLabel(size_t nLabelIndex)
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
/* 消去 */
void CLilyanSceneCrafter::clearScenarioData()
{
	m_textData.clear();

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_images.clear();
	m_nImageIndex = 0;

	m_sceneTitle.clear();

	m_soundData.clear();

	m_labelData.clear();
}

ID2D1Bitmap* CLilyanSceneCrafter::importWholeImage(const std::wstring& iImageFilePath)
{
	ID2D1Bitmap* p = nullptr;

	win_image::SImageFrame imageFrame{};
	bool bRet = win_image::LoadImageToMemory(iImageFilePath.c_str(), &imageFrame);
	if (bRet)
	{
		CComPtr<ID2D1Bitmap> pD2d1Bitmap;

		HRESULT hr = m_pStoredD2d1DeviceContext->CreateBitmap(
			D2D1::SizeU(imageFrame.uiWidth, imageFrame.uiHeight),
			D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&pD2d1Bitmap);

		D2D1_RECT_U rc = { 0, 0, imageFrame.uiWidth, imageFrame.uiHeight };
		hr = pD2d1Bitmap->CopyFromMemory(&rc, imageFrame.pixels.data(), imageFrame.uiStride);
		if (SUCCEEDED(hr))
		{
			m_images.push_back(std::move(pD2d1Bitmap));
			p = m_images.back().p;
		}
	}

	return p;
}
