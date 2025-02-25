#ifndef LILYAN_H_
#define LILYAN_H_

#include <string>
#include <vector>

#include "adv.h"

namespace lilyan
{
	bool LoadScenario(
		const std::wstring& wstrScriptFilePath,
		std::vector<adv::TextDatum>& textData,
		std::vector<std::wstring>& imageFilePaths,
		std::vector<adv::SceneDatum>& sceneData,
		std::wstring &wstrTitle,
		std::vector<adv::SoundDatum>& soundData
	);
}

#endif // !LILYAN_H_
