#include "pch.h"
#include "global.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <unordered_set>

#include "skipFunctions.h"

#include "cxxopts.hpp"

#include <llvm/Demangle/Demangle.h>
#include <llvm/Demangle/MicrosoftDemangle.h>

#include "pdb.h"

#include "../PortableExecutable/pe_bliss.h"

#include <windows.h>

#include "PeEdit.h"

using namespace pe_bliss;
namespace LLPE {
	std::string bedrockExeName = "bedrock_server.exe";
	std::string LiteModExeName = "bedrock_server_mod.exe";
	std::string bedrockPdbName = "bedrock_server.pdb";
	std::string ApiFileName = "bedrock_server_api.def";
	std::string VarFileName = "bedrock_server_var.def";
	std::string SymFileName = "bedrock_server_symlist.txt";
	std::deque<PdbSymbol> filteredFunctionList;

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

	bool InitializeFunctionList(bool genSymList = false) {
		std::ofstream BDSSymList;
		auto FunctionList = loadPDB(str2wstr(bedrockPdbName).c_str());
		if (FunctionList == NULL) {
			return false;
		}
		if (genSymList) {
			std::cout << "[Info] Generating symbol list, please wait..." << std::endl;
			BDSSymList.open(SymFileName, std::ios::ate | std::ios::out);
			if (!BDSSymList) {
				std::cout << "[Error][PeEditor] Cannot create " << SymFileName << std::endl;
				return false;
			}
		}
		else if (filteredFunctionList.size() == 0) {
			std::cout << "[Info] Filtering PDB symbols, please wait..." << std::endl;
		}
		if (filteredFunctionList.size() == 0 || genSymList) {
			for (const auto& fn : *FunctionList) {
				bool keepFunc = true;
				if (genSymList) {
					char tmp[11];
					sprintf_s(tmp, 11, "[%08d]", fn.Rva);

					auto demangledName = llvm::microsoftDemangle(
						fn.Name.c_str(), nullptr, nullptr, nullptr, nullptr,
						(llvm::MSDemangleFlags)(llvm::MSDF_NoCallingConvention | llvm::MSDF_NoAccessSpecifier));
					if (demangledName) {
						BDSSymList << demangledName << std::endl;
						free(demangledName);
					}
					BDSSymList << tmp << fn.Name << std::endl << std::endl;
					continue;
				}
				if (fn.Name[0] != '?') {
					keepFunc = false;
				}
				for (const std::string a : LLTool::SkipPerfix) {
					if (fn.Name.starts_with(a)) {
						keepFunc = false;
					}
				}
				for (const auto& reg : LLTool::SkipRegex) {
					std::smatch result;
					if (std::regex_match(fn.Name, result, reg)) {
						keepFunc = false;
					}
				}
				if (keepFunc && !genSymList) {
					filteredFunctionList.push_back(fn);
				}
			}
		}
		if (BDSSymList.is_open()) {
			try {
				BDSSymList.flush();
				BDSSymList.close();
				std::cout << "[Info] [DEV]Symbol List File              Created" << std::endl;
			}
			catch (...) {
				std::cout << "[Error] Failed to Create " << SymFileName << std::endl;
			}
		}
	}

	bool LLPEAPI ProcessFunctionList() {
		if (!InitializeFunctionList()) {
			std::cout << "[Error][PeEditor] Error processing function list from PDB!" << std::endl;
			return false;
		}
		return true;
	}

	bool LLPEAPI CreateSymbolList() {
		if (!InitializeFunctionList(true)) {
			std::cout << "[Error][PeEditor] Error processing function list from PDB!" << std::endl;
			return false;
		}
	}

	void LLPEAPI SetEditorFilename(int fileType, const char* desiredName) {
		switch (fileType)
		{
		case LLFileTypes::VanillaExe:
			bedrockExeName = desiredName;
			break;
		case LLFileTypes::LiteModExe:
			LiteModExeName = desiredName;
			break;
		case LLFileTypes::BedrockPdb:
			bedrockPdbName = desiredName;
			break;
		case LLFileTypes::ApiDef:
			ApiFileName = desiredName;
			break;
		case LLFileTypes::VarDef:
			VarFileName = desiredName;
			break;
		case LLFileTypes::SymbolList:
			SymFileName = desiredName;
			break;
		default:
			break;
		}
	}

	extern LLPEAPI char* GetEditorFilename(int fileType) {
		switch (fileType)
		{
		case LLFileTypes::VanillaExe:
			return (char*)bedrockExeName.c_str();
			break;
		case LLFileTypes::LiteModExe:
			return (char*)LiteModExeName.c_str();
			break;
		case LLFileTypes::BedrockPdb:
			return (char*)bedrockPdbName.c_str();
			break;
		case LLFileTypes::ApiDef:
			return (char*)ApiFileName.c_str();
			break;
		case LLFileTypes::VarDef:
			return (char*)VarFileName.c_str();
			break;
		case LLFileTypes::SymbolList:
			return (char*)SymFileName.c_str();
			break;
		default:
			return (char*)"";
			break;
		}
	}

	bool LLPEAPI ProcessLibFile(const char* libName) {
		std::ifstream libFile;
		std::ofstream outLibFile;

		libFile.open(libName, std::ios::in | std::ios::binary);
		if (!libFile) {
			std::cout << "[Err] Cannot open " << libName << std::endl;
			return false;
		}
		bool saveFlag = false;
		pe_base* LiteLib_PE = new pe_base(pe_factory::create_pe(libFile));
		try {
			imported_functions_list imports(get_imported_functions(*LiteLib_PE));
			for (int i = 0; i < imports.size(); i++) {
				if (imports[i].get_name() == "bedrock_server_mod.exe") {
					imports[i].set_name(bedrockExeName);
					std::cout << "[INFO] Modding dll file " << libName << std::endl;
					saveFlag = true;
				}
			}

			if (saveFlag) {
				section ImportSection;
				ImportSection.get_raw_data().resize(1);
				ImportSection.set_name("ImpFunc");
				ImportSection.readable(true).writeable(true);
				section& attachedImportedSection = LiteLib_PE->add_section(ImportSection);
				pe_bliss::rebuild_imports(*LiteLib_PE, imports, attachedImportedSection, import_rebuilder_settings(true, false));

				outLibFile.open("LiteLoader.tmp", std::ios::out | std::ios::binary | std::ios::trunc);
				if (!outLibFile) {
					std::cout << "[Err] Cannot create LiteLoader.tmp!" << std::endl;
					return false;
				}
				std::cout << "[INFO] Writting dll file " << libName << std::endl;
				pe_bliss::rebuild_pe(*LiteLib_PE, outLibFile);
				libFile.close();
				std::filesystem::remove(std::filesystem::path(libName));
				outLibFile.close();
				std::filesystem::rename(std::filesystem::path("LiteLoader.tmp"), std::filesystem::path(libName));
			}
			if (libFile.is_open()) libFile.close();
			if (outLibFile.is_open()) outLibFile.close();
		}
		catch (const pe_exception& e) {
			std::cout << "[Err] Set ImportName Failed: " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	bool LLPEAPI ProcessLibDirectory(const char* directoryName) {
		std::filesystem::directory_iterator directory(directoryName);

		for (auto& file : directory) {
			if (!file.is_regular_file()) {
				continue;
			}
			std::filesystem::path path = file.path();
			std::u8string u8Name = path.filename().u8string();
			std::u8string u8Ext = path.extension().u8string();
			std::string name = reinterpret_cast<std::string&>(u8Name);
			std::string ext = reinterpret_cast<std::string&>(u8Ext);

			// Check is dll
			if (ext != ".dll") {
				continue;
			}

			std::string pluginName;
			pluginName.append(directoryName).append("\\").append(name);
			if (!ProcessLibFile(pluginName.c_str())) return false;
		}
		return true;
	}

	bool LLPEAPI ProcessPlugins() {
		if (ProcessLibFile("LiteLoader.dll") && ProcessLibDirectory("plugins") && ProcessLibDirectory("plugins\\LiteLoader")) {
			return true;
		}
		return false;
	}

	bool LLPEAPI GenerateDefinitionFiles() {
		std::ofstream BDSDef_API;
		std::ofstream BDSDef_VAR;
		if(InitializeFunctionList()) {
			std::cout << "[Error] Could not process symbols from PDB!" << std::endl;
			return false;
		}

		BDSDef_API.open(ApiFileName, std::ios::ate | std::ios::out);
		if (!BDSDef_API) {
			std::cout << "[Error][PeEditor] Cannot create bedrock_server_api.def" << std::endl;
			return false;
		}
		BDSDef_API << "LIBRARY bedrock_server.dll\nEXPORTS\n";

		BDSDef_VAR.open(VarFileName, std::ios::ate | std::ios::out);
		if (!BDSDef_VAR) {
			std::cout << "[Error][PeEditor] Failed to create " << VarFileName << std::endl;
			return false;
		}
		BDSDef_VAR << "LIBRARY " << bedrockExeName << "\nEXPORTS\n";
		for (auto& fn : filteredFunctionList) {
			if (fn.IsFunction) {
				BDSDef_API << "\t" << fn.Name << "\n";
			}
			else
				BDSDef_VAR << "\t" << fn.Name << "\n";
		}
		try {
			BDSDef_API.flush();
			BDSDef_API.close();
			BDSDef_VAR.flush();
			BDSDef_VAR.close();
			std::cout << "[Info] [DEV]bedrock_server_var/api def    Created" << std::endl;
		}
		catch (...) {
			std::cout << "[Error][PeEditor] Failed to Create " << ApiFileName << std::endl;
			return false;
		}
		return true;
	}

	bool LLPEAPI CreateModifiedExecutable() {
		std::ifstream             OriginalBDS;
		std::ifstream			  LiteLib;
		std::ofstream             ModifiedBDS;
		std::ofstream             ModdedLiteLib;
		pe_base* OriginalBDS_PE = nullptr;
		pe_base* LiteLib_PE = nullptr;
		export_info                OriginalBDS_ExportInf;
		exported_functions_list         OriginalBDS_ExportFunc;
		uint16_t                  ExportLimit = 0;
		int                       ExportCount = 1;
		int                       ApiDefCount = 1;
		int                       ApiDefFileCount = 1;

		if (!InitializeFunctionList()) {
			std::cout << "[Error] Could not process symbols from PDB!" << std::endl;
			return false;
		}
		OriginalBDS.open(bedrockExeName, std::ios::in | std::ios::binary);
		if (!OriginalBDS) {
			std::cout << "[Error][PeEditor] Cannot open bedrock_server.exe" << std::endl;
			return false;
		}
		ModifiedBDS.open(LiteModExeName, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!ModifiedBDS) {
			std::cout << "[Error][PeEditor] Cannot open " << LiteModExeName << std::endl;
			return false;
		}

		OriginalBDS_PE = new pe_base(pe_factory::create_pe(OriginalBDS));
		try {
			OriginalBDS_ExportFunc = get_exported_functions(*OriginalBDS_PE, OriginalBDS_ExportInf);
		}
		catch (const pe_exception& e) {
			std::cout << "[Error][pe_bliss] " << e.what() << std::endl;
			return false;
		}
		ExportLimit = get_export_ordinal_limits(OriginalBDS_ExportFunc).second;

		for (const auto& fn : filteredFunctionList) {
			try {
				if (!fn.IsFunction) {
					exported_function func;
					func.set_name(fn.Name);
					func.set_rva(fn.Rva);
					func.set_ordinal(ExportLimit + ExportCount);
					ExportCount++;
					if (ExportCount > 65535) {
						std::cout << "[Error][pe_bliss] Too many Symbols are going to insert to ExportTable" << std::endl;
						return false;
					}
					OriginalBDS_ExportFunc.push_back(func);
				}
			}
			catch (const pe_exception& e) {
				std::cout << "[Error][pe_bliss] " << e.what() << std::endl;
				return false;
			}
			catch (...) {
				std::cout << "UnkErr" << std::endl;
				return false;
			}
		}
		try {
			section ExportSection;
			ExportSection.get_raw_data().resize(1);
			ExportSection.set_name("ExpFunc");
			ExportSection.readable(true);
			section& attachedExportedSection = OriginalBDS_PE->add_section(ExportSection);
			rebuild_exports(*OriginalBDS_PE, OriginalBDS_ExportInf, OriginalBDS_ExportFunc, attachedExportedSection);

			imported_functions_list imports(get_imported_functions(*OriginalBDS_PE));

			import_library preLoader;
			preLoader.set_name("LLPreLoader.dll");

			imported_function func;
			func.set_name("dlsym_real");
			func.set_iat_va(0x1);

			preLoader.add_import(func);
			imports.push_back(preLoader);

			section ImportSection;
			ImportSection.get_raw_data().resize(1);
			ImportSection.set_name("ImpFunc");
			ImportSection.readable(true).writeable(true);
			section& attachedImportedSection = OriginalBDS_PE->add_section(ImportSection);
			rebuild_imports(*OriginalBDS_PE, imports, attachedImportedSection, import_rebuilder_settings(true, false));

			rebuild_pe(*OriginalBDS_PE, ModifiedBDS);
			ModifiedBDS.close();
		}
		catch (pe_exception e) {
			std::cout << "[Error][PeEditor] Failed to rebuild " << LiteModExeName << std::endl;
			std::cout << "[Error][pe_bliss] " << e.what() << std::endl;
			ModifiedBDS.close();
			std::filesystem::remove(std::filesystem::path(LiteModExeName));
		}
		catch (...) {
			std::cout << "[Error][PeEditor] Failed to rebuild " << LiteModExeName << " with unk err" << std::endl;
			ModifiedBDS.close();
			std::filesystem::remove(std::filesystem::path(LiteModExeName));
		}
		return true;
	}

	int LLPEAPI GetFilteredFunctionListCount() {
		if (filteredFunctionList.size() == 0) {
			if (InitializeFunctionList()) {
				std::cout << "[Error] Could not process symbols from PDB!" << std::endl;
				return 0;
			}
		}
		return (int)filteredFunctionList.size();
	}
}