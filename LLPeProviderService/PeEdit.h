#pragma once
#include "pch.h"
#include <Windows.h>

namespace LLPE {
	bool LLPEAPI ProcessFunctionList(const char* pdbFile);
	bool LLPEAPI CreateSymbolList(const char* symFileName, const char* pdbFile);
	bool LLPEAPI ProcessLibFile(const char* libName, const char* modExeName);
	bool LLPEAPI ProcessLibDirectory(const char* directoryName, const char* modExeName);
	bool LLPEAPI ProcessPlugins(const char* modExeName);
	bool LLPEAPI GenerateDefinitionFiles(const char* pdbName, const char* apiName, const char* varName);
	bool LLPEAPI CreateModifiedExecutable(const char* bedrockName, const char* liteModName, const char* pdbName);
	int  LLPEAPI GetFilteredFunctionListCount(const char* pdbName);
}
