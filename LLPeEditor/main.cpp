#include "pch.h"
#include <string>
#include <windows.h>

#include "cxxopts.hpp"

#include "..\LLPeProviderService\PeEdit.h"

using namespace LLPE;

std::wstring str2wstr(const std::string& str, UINT codePage = CP_UTF8) {
	auto len = MultiByteToWideChar(codePage, 0, str.c_str(), -1, nullptr, 0);
	auto* buffer = new wchar_t[len + 1];
	MultiByteToWideChar(codePage, 0, str.c_str(), -1, buffer, len + 1);
	buffer[len] = L'\0';

	std::wstring result(buffer);
	delete[] buffer;
	return result;
}

inline void Pause(bool Pause) {
	if (Pause)
		system("pause");
}

int main(int argc, char** argv) {
	if (argc == 1){
		SetConsoleCtrlHandler([](DWORD signal) -> BOOL {return TRUE; }, TRUE);
		EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE,
			MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	cxxopts::Options options("LLPeEditor", "BDSLiteLoader PE Editor For BDS");
	options.allow_unrecognised_options();
	options.add_options()
		("noMod", "Do not generate modded executable")
		("d,def", "Generate def files for develop propose")
		("q,noPause", "Do not pause before exit")
		("p,pluginsOnly", "Only process olugin files for alternate executable name")
		("defApi", "Def File name for API Definitions", cxxopts::value<std::string>()
			->default_value("bedrock_server_api.def"))
		("defVar", "Def File name for Variable Definitions", cxxopts::value<std::string>()
			->default_value("bedrock_server_var.def"))
		("s,sym", "Generate symlist that contains symbol and it's rva", cxxopts::value<std::string>()
			->default_value("bedrock_server_symlist.txt")
			->implicit_value("bedrock_server_symlist.txt"))
		("exe", "BDS executeable file name", cxxopts::value<std::string>()
			->default_value("bedrock_server.exe"))
		("o,out", "LL executeable output file name", cxxopts::value<std::string>()
			->default_value("bedrock_server_mod.exe"))
		("pdb", "BDS debug database file name", cxxopts::value<std::string>()
			->default_value("bedrock_server.pdb"))
		("h,help", "Print usage");
	auto optionsResult = options.parse(argc, argv);

	if (optionsResult.count("help")){
		std::cout << options.help() << std::endl;
		exit(0);
	}

	bool mGenModBDS = !optionsResult["noMod"].as<bool>();
	bool mGenDevDef = optionsResult["def"].as<bool>();
	bool mGenSymbolList = optionsResult.count("sym");
	bool mShouldPause = !optionsResult["noPause"].as<bool>();
	bool mPluginsOnly = optionsResult["pluginsOnly"].as<bool>();
	
	std::string mExeFile = optionsResult["exe"].as<std::string>();
	std::string mOutputExeFile = optionsResult["out"].as<std::string>();
	std::string mPdbFile = optionsResult["pdb"].as<std::string>();
	std::string mDefApiFile = optionsResult["defApi"].as<std::string>();
	std::string mDefVarFile = optionsResult["defVar"].as<std::string>();
	std::string mSymFile = optionsResult["sym"].as<std::string>();

	std::cout << "[Info] LiteLoader ToolChain PEEditor" << std::endl;
	std::cout << "[Info] BuildDate CST " __TIMESTAMP__ << std::endl;
	std::cout << "[Info] Gen mod executable                      " << std::boolalpha << std::string(mGenModBDS ? " true" : "false") << std::endl;
	std::cout << "[Info] Gen bedrock_server defs           [DEV] " << std::boolalpha << std::string(mGenDevDef ? " true" : "false") << std::endl;
	std::cout << "[Info] Gen SymList file                  [DEV] " << std::boolalpha << std::string(mGenSymbolList ? " true" : "false") << std::endl;
	std::cout << "[Info] Fix Plugins                       [DEV] " << std::boolalpha << std::string(mPluginsOnly ? " true" : "false") << std::endl;
	std::cout << "[Info] Output: " << mOutputExeFile << std::endl;

	if (mGenSymbolList) {
		if (!CreateSymbolList(mSymFile.c_str(), mPdbFile.c_str())) {
			Pause(mShouldPause);
			return -1;
		}
	}
	if(!ProcessFunctionList(mPdbFile.c_str())) {
		Pause(mShouldPause);
		return -1;
	}

	std::cout << "[Info] Loaded " << GetFilteredFunctionListCount(mPdbFile.c_str()) << " Symbols" << std::endl;
	std::cout << "[Info] Generating BDS. Please wait for few minutes" << std::endl;

	if (mPluginsOnly) {
		if (ProcessLibFile("LiteLoader.dll", mOutputExeFile.c_str()) && ProcessLibDirectory("plugins", mOutputExeFile.c_str()) && ProcessLibDirectory("plugins\\LiteLoader", mOutputExeFile.c_str())) {
			std::cout << "[Info] Plugins have been fixed." << std::endl;
			Pause(mShouldPause);
			return 0;
		}
		else {
			std::cout << "[Error] There was am error fixing plugins!" << std::endl;
			Pause(mShouldPause);
			return -1;
		}
	}
	if (mGenDevDef) {
		std::cout << "[Info] Generating definiton files, please wait..." << std::endl;
		GenerateDefinitionFiles(mPdbFile.c_str(), mDefApiFile.c_str(), mDefVarFile.c_str());
	}
	if (mGenModBDS) {
		if (CreateModifiedExecutable(mExeFile.c_str(), mOutputExeFile.c_str(), mPdbFile.c_str())) {
			std::cout << "[Info] Modified executable created successfully!" << std::endl;
		}
	}
	Pause(mShouldPause);
	return 0;
}
