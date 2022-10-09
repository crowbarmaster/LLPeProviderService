#pragma once
#include "pch.h"
#include <Windows.h>

namespace LLPE {
	bool LLPEAPI ProcessFunctionList();
	bool LLPEAPI CreateSymbolList();
	void LLPEAPI SetEditorFilename(int fileType, const char* desiredName);
	extern LLPEAPI const char* GetEditorFilename(int fileType);
	bool LLPEAPI ProcessLibFile(const char* libName);
	bool LLPEAPI ProcessLibDirectory(const char* directoryName);
	bool LLPEAPI ProcessPlugins();
	bool LLPEAPI GenerateDefinitionFiles();
	bool LLPEAPI CreateModifiedExecutable();
	int LLPEAPI GetFilteredFunctionListCount();
}
