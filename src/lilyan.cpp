

#include "lilyan.h"

#include "win_filesystem.h"
#include "win_dialogue.h"
#include "win_text.h"
#include "text_utility.h"

#include "deps/nlohmann/json.hpp"

/*内部用*/
namespace lilyan
{
	struct SResourcePath
	{
		std::wstring wstrStillFolderPath;
		std::wstring wstrVoiceFolderPath;
		std::wstring wstrSoundFolderPath;
	};

	/*各素材経路導出*/
	static bool DeriveResourcePathFromScriptFilePath(const std::wstring& wstrScriptFilePath, SResourcePath& resourcePath)
	{
		const wchar_t g_swzScriptFolderName[] = L"Scripts";
		size_t nPos = wstrScriptFilePath.find(g_swzScriptFolderName);
		if (nPos == std::wstring::npos)return false;

		std::wstring wstrBaseFolder = wstrScriptFilePath.substr(0, nPos);

		resourcePath.wstrStillFolderPath = wstrBaseFolder + L"Backgrounds\\MainBackground\\";
		resourcePath.wstrVoiceFolderPath = wstrBaseFolder + L"Voice\\";
		resourcePath.wstrSoundFolderPath = wstrBaseFolder + L"Audio\\Sfx\\";

		return true;
	}

	enum class ETokenType
	{
		Unknown = -1,
		Comment,
		Voice,
		Text,
		Bg,
		Music,
		Sound
	};

	struct STokenDatum
	{
		ETokenType tokenType = ETokenType::Unknown;
		std::string strData;
	};

	/*台本解析*/
	static void ParseScenario(const std::string& strFile, std::vector<STokenDatum> &tokenData, std::string &strError)
	{
		try
		{
			const nlohmann::json nlJson = nlohmann::json::parse(strFile);

			const auto& refIds = nlJson.at("references").at("RefIds");
			for (const auto& refId : refIds)
			{
				STokenDatum tokenDatum;

				const auto& commandType = refId.at("type").at("class");
				const auto& commandData = refId.at("data");

				if (commandType == "CommentScriptLine")
				{
					tokenDatum.tokenType = ETokenType::Comment;
					tokenDatum.strData = commandData.at("commentText");
				}
				if (commandType == "PlayVoice")
				{
					tokenDatum.tokenType = ETokenType::Voice;
					tokenDatum.strData = commandData.at("VoicePath").at("value");
				}
				else if (commandType == "PrintText")
				{
					tokenDatum.tokenType = ETokenType::Text;

					const std::string& auctor = commandData.at("AuthorId").at("value");
					if (!auctor.empty())
					{
						tokenDatum.strData = auctor;
						tokenDatum.strData += ":";
					}

					std::string text = commandData.at("Text").at("value");
					if (text.empty())
					{
						text = commandData.at("Text").at("dynamicValue").at("ValueText");
						if (text.empty())continue;
					}

					tokenDatum.strData += " \n";
					tokenDatum.strData += text;
				}
				else if (commandType == "ModifyBackground")
				{
					tokenDatum.tokenType = ETokenType::Bg;
					tokenDatum.strData = commandData.at("AppearanceAndTransition").at("value").at("name").at("value");
				}
				else if (commandType == "PlaySfx")
				{
					tokenDatum.tokenType = ETokenType::Sound;
					tokenDatum.strData = commandData.at("SfxPath").at("value");
				}

				if (tokenDatum.tokenType != ETokenType::Unknown)
				{
					tokenData.push_back(std::move(tokenDatum));
				}
			}
		}
		catch (nlohmann::json::exception e)
		{
			strError = e.what();
		}
	}

	/*背景画像一覧作成*/
	static void CreateBgFilePaths(const std::wstring &wstrBaseBgFolderPath, const std::wstring& wstrBgFileName, std::vector<std::wstring>& BgFilePaths)
	{
		std::wstring wstrEventFolderPath = wstrBaseBgFolderPath + text_utility::ExtractDirectory(wstrBgFileName);

		win_filesystem::CreateFilePathList(wstrEventFolderPath.c_str(), L".png", BgFilePaths);
	}

	/*背景画像番号検索*/
	static long long FindBgIndex(const std::vector<std::wstring>& BgFilePaths, const std::wstring& wstrBgFileName)
	{
		const auto& iter = std::find_if
		(
			BgFilePaths.begin(), BgFilePaths.end(),
			[&wstrBgFileName](const std::wstring& wstrFilePath)
			{
				return wstrFilePath.find(wstrBgFileName) != std::wstring::npos;
			}
		);

		if (iter != BgFilePaths.cend())
		{
			return std::distance(BgFilePaths.begin(), iter);
		}

		return -1;
	}

} /* namespace lilyan */

/*台本読み込み*/
bool lilyan::LoadScenario(const std::wstring& wstrScriptFilePath, std::vector<adv::TextDatum>& textData, std::vector<std::wstring>& imageFilePaths, std::vector<adv::SceneDatum>& sceneData, std::wstring& wstrTitle, std::vector<adv::SoundDatum>& soundData, std::vector <adv::LabelDatum>& labelData)
{
	std::string strFile = win_filesystem::LoadFileAsString(wstrScriptFilePath.c_str());
	if (strFile.empty())return false;

	SResourcePath resourcePath;
	bool bRet = DeriveResourcePathFromScriptFilePath(wstrScriptFilePath, resourcePath);
	if (!bRet)return false;

	std::vector<STokenDatum> tokenData;
	std::string strError;
	ParseScenario(strFile, tokenData, strError);

	if (!strError.empty())
	{
		win_dialogue::ShowMessageBox("Parse error", strError.c_str());
		return false;
	}

	std::vector<std::wstring> wstrBgFilePaths;

	std::wstring wstrVoiceFileNameBuffer;
	std::wstring wstrSoundFileNameBuffer;
	adv::SceneDatum sceneDatumBuffer;
	std::wstring labelBuffer;

	for (const auto& tokenDatum : tokenData)
	{
		const auto& tokenType = tokenDatum.tokenType;
		if (tokenType == ETokenType::Comment)
		{
			if (wstrTitle.empty())
			{
				wstrTitle = win_text::WidenUtf8(tokenDatum.strData);
			}
		}
		else if (tokenType == ETokenType::Voice)
		{
			wstrVoiceFileNameBuffer = resourcePath.wstrVoiceFolderPath + win_text::WidenUtf8(tokenDatum.strData) + L".m4a";
		}
		else if (tokenType == ETokenType::Text)
		{
			adv::TextDatum textDatum;
			textDatum.wstrText = win_text::WidenUtf8(tokenDatum.strData);
			text_utility::ReplaceAll(textDatum.wstrText, L"{G_PlayerName}", L"主人公");

			/*効果音と音声が重なる場合、文章データを複製して間を持たせる。*/
			if (!wstrSoundFileNameBuffer.empty())
			{
				if (!wstrVoiceFileNameBuffer.empty())
				{
					textData.emplace_back(adv::TextDatum{ textDatum.wstrText , L"" });

					sceneDatumBuffer.nTextIndex = textData.size() - 1;
					sceneData.push_back(sceneDatumBuffer);
				}

				adv::SoundDatum soundDatum;
				soundDatum.wstrSoundFilePath = wstrSoundFileNameBuffer;
				soundDatum.nSceneIndex = sceneData.size() - 1;
				soundData.push_back(std::move(soundDatum));

				wstrSoundFileNameBuffer.clear();
			}

			if (!wstrVoiceFileNameBuffer.empty())
			{
				textDatum.wstrVoicePath = wstrVoiceFileNameBuffer;
				wstrVoiceFileNameBuffer.clear();
			}

			textData.push_back(std::move(textDatum));

			sceneDatumBuffer.nTextIndex = textData.size() - 1;
			sceneData.push_back(sceneDatumBuffer);

			if (!labelBuffer.empty())
			{
				labelData.emplace_back(adv::LabelDatum{ labelBuffer, sceneData.size() - 1 });
				labelBuffer.clear();
			}
		}
		else if (tokenType == ETokenType::Bg)
		{
			if (tokenDatum.strData.find("Event") == std::string::npos)continue;

			std::wstring wstrBgName = win_text::WidenUtf8(tokenDatum.strData);

			if (wstrBgFilePaths.empty())
			{
				CreateBgFilePaths(resourcePath.wstrStillFolderPath, wstrBgName, wstrBgFilePaths);
				if (wstrBgFilePaths.empty())return false;
			}

			std::wstring wstrFileIndex = text_utility::ExtractFileName(wstrBgName);
			unsigned long ulIndex = wcstoul(wstrFileIndex.c_str(), nullptr, 10) - 1L;
			if (ulIndex >= wstrBgFilePaths.size())
			{
				/*何故か14からの指定なので減算する。*/
				if (wstrScriptFilePath.rfind(L"chara1022_203.nani") != std::wstring::npos)
				{
					ulIndex -= 3L;
				}

				if (ulIndex >= wstrBgFilePaths.size())return false;
			}

			imageFilePaths.push_back(wstrBgFilePaths[ulIndex]);
			sceneDatumBuffer.nImageIndex = imageFilePaths.size() - 1;

			labelBuffer = text_utility::ExtractFileName(wstrBgFilePaths[ulIndex]);
		}
		else if(tokenType == ETokenType::Sound)
		{
			/*名称指定に拡張子有無の表記揺れ有り。*/
			std::wstring wstrFileName = text_utility::ExtractFileName(win_text::WidenUtf8(tokenDatum.strData));
			wstrSoundFileNameBuffer = resourcePath.wstrSoundFolderPath + wstrFileName + L".m4a";
		}
	}

	return true;
}
