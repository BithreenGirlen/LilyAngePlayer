

#include "lilyan.h"

#include "win_filesystem.h"
#include "win_dialogue.h"
#include "win_text.h"
#include "text_utility.h"
#include "path_utility.h"

#include "deps/nlohmann/json.hpp"

/* 内部用 */
namespace lilyan
{
	struct SResourcePath
	{
		std::wstring stillFolderPath;
		std::wstring voiceFolderPath;
		std::wstring soundFolderPath;
	};

	/* 各素材経路導出 */
	static bool DeriveResourcePathFromScriptFilePath(const std::wstring& scriptFilePath, SResourcePath& resourcePath)
	{
		static constexpr std::wstring_view scriptFolderName = L"Scripts";
		size_t nPos = scriptFilePath.find(scriptFolderName);
		if (nPos == std::wstring::npos)return false;

		resourcePath.stillFolderPath.assign(&scriptFilePath[0], nPos).append(L"Backgrounds\\MainBackground\\");
		resourcePath.voiceFolderPath.assign(&scriptFilePath[0], nPos).append(L"Voice\\");
		resourcePath.soundFolderPath.assign(&scriptFilePath[0], nPos).append(L"Audio\\Sfx\\");

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

	/* 台本解析 */
	static bool ParseScenario(const std::string& file, std::vector<STokenDatum> &tokenData)
	{
		try
		{
			const nlohmann::json nlJson = nlohmann::json::parse(file);

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
		catch (const nlohmann::json::exception& e)
		{
			win_dialogue::ShowMessageBox("Parse error", e.what());
			return false;
		}

		return true;
	}

	/* 背景画像一覧作成 */
	static void CreateBgFilePaths(const std::wstring &bgBaseFolderPath, const std::wstring& bgFileName, std::vector<std::wstring>& BgFilePaths)
	{
		std::wstring eventFolderPath = bgBaseFolderPath + std::wstring(path_utility::ExtractParentPath(bgFileName));

		win_filesystem::CreateFilePathList(eventFolderPath, L".png", BgFilePaths);
	}

	/// @brief 文字列の8バイト整数への変換
	template<typename CharType>
	static uint64_t StrToUInt64(const std::basic_string_view<CharType>& s)
	{
		if constexpr (std::is_same_v<CharType, wchar_t>)
		{
			/* The length of uint64_t is 20 characters at most. */
			wchar_t uint64Buffer[32]{};
			constexpr size_t bufferLength = sizeof(uint64Buffer) / sizeof(wchar_t) - 1;
			if (s.length() > bufferLength)return static_cast<uint64_t>(-1LL);

			::memcpy(uint64Buffer, s.data(), s.length() * sizeof(wchar_t));
			uint64Buffer[s.length()] = L'\0';

			return ::wcstoull(uint64Buffer, nullptr, 10);
		}
		else if constexpr (std::is_same_v<CharType, char>)
		{
			char uint64Buffer[32]{};
			constexpr size_t bufferLength = sizeof(uint64Buffer) - 1;
			if (s.length() > bufferLength)return static_cast<uint64_t>(-1LL);

			::memcpy(uint64Buffer, s.data(), s.length());
			uint64Buffer[s.length()] = '\0';

			return ::strtoull(uint64Buffer, nullptr, 10);
		}
	}

} /* namespace lilyan */

/* 台本読み込み */
bool lilyan::LoadScenario(const std::wstring& scriptFilePath, std::vector<adv::TextDatum>& textData, std::vector<std::wstring>& imageFilePaths, std::vector<adv::SceneDatum>& sceneData, std::wstring& sceneTitle, std::vector<adv::SoundDatum>& soundData, std::vector <adv::LabelDatum>& labelData)
{
	std::string scriptFile = win_filesystem::LoadFileAsString(scriptFilePath.c_str());
	if (scriptFile.empty())return false;

	SResourcePath resourcePath;
	bool bRet = DeriveResourcePathFromScriptFilePath(scriptFilePath, resourcePath);
	if (!bRet)return false;

	std::vector<STokenDatum> tokenData;
	bRet = ParseScenario(scriptFile, tokenData);
	if (!bRet)return false;

	std::vector<std::wstring> bgFilePaths;

	std::wstring voiceFileNameBuffer;
	std::wstring soundFileNameBuffer;
	adv::SceneDatum sceneDatumBuffer;
	std::wstring labelBuffer;

	for (const auto& tokenDatum : tokenData)
	{
		const auto& tokenType = tokenDatum.tokenType;
		if (tokenType == ETokenType::Comment)
		{
			if (sceneTitle.empty())
			{
				sceneTitle = win_text::WidenUtf8(tokenDatum.strData);
			}
		}
		else if (tokenType == ETokenType::Voice)
		{
			voiceFileNameBuffer.assign(resourcePath.voiceFolderPath).append(win_text::WidenUtf8(tokenDatum.strData)).append(L".m4a");
		}
		else if (tokenType == ETokenType::Text)
		{
			adv::TextDatum textDatum;
			textDatum.message = win_text::WidenUtf8(tokenDatum.strData);
			text_utility::ReplaceAll(textDatum.message, L"{G_PlayerName}", L"主人公");

			/* 効果音と音声が重なる場合、文章データを複製して間を持たせる。 */
			if (!soundFileNameBuffer.empty())
			{
				if (!voiceFileNameBuffer.empty())
				{
					textData.emplace_back(adv::TextDatum{ textDatum.message , L"" });

					sceneDatumBuffer.nTextIndex = textData.size() - 1;
					sceneData.push_back(sceneDatumBuffer);
				}

				adv::SoundDatum soundDatum
				{
					.nSceneIndex = sceneData.size() - 1,
					.soundFilePath = soundFileNameBuffer
				};

				soundData.push_back(std::move(soundDatum));

				soundFileNameBuffer.clear();
			}

			if (!voiceFileNameBuffer.empty())
			{
				textDatum.voiceFilePath = voiceFileNameBuffer;
				voiceFileNameBuffer.clear();
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

			std::wstring bgRelativeFilePath = win_text::WidenUtf8(tokenDatum.strData);

			if (bgFilePaths.empty())
			{
				CreateBgFilePaths(resourcePath.stillFolderPath, bgRelativeFilePath, bgFilePaths);
				if (bgFilePaths.empty())return false;
			}

			uint64_t fileIndex = StrToUInt64(path_utility::ExtractFileNameWithoutExtension(bgRelativeFilePath));
			if (fileIndex == static_cast<uint64_t>(-1LL) || fileIndex == 0)return false;
			--fileIndex;
			if (fileIndex >= bgFilePaths.size())
			{
				/* 何故か14からの指定なので減算する。 */
				if (scriptFilePath.ends_with(L"chara1022_203.nani"))
				{
					fileIndex -= 3LL;
				}

				if (fileIndex >= bgFilePaths.size())return false;
			}

			imageFilePaths.push_back(bgFilePaths[fileIndex]);
			sceneDatumBuffer.nImageIndex = imageFilePaths.size() - 1;

			labelBuffer = path_utility::ExtractFileNameWithoutExtension(bgFilePaths[fileIndex]);
		}
		else if(tokenType == ETokenType::Sound)
		{
			/* 名称指定に拡張子有無の表記揺れ有り。 */
			std::wstring_view fileName = path_utility::ExtractFileNameWithoutExtension(win_text::WidenUtf8(tokenDatum.strData));
			soundFileNameBuffer.assign(resourcePath.soundFolderPath).append(fileName).append(L".m4a");
		}
	}

	return true;
}
