#undef __STRICT_ANSI__
#include <windows.h>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <cstdio>

#include <stdio.h>  /* defines FILENAME_MAX */
#include <direct.h>
#define GetCurrentDir _getcwd

#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/schema.h"

#define DLL_EXPORT __declspec(dllexport)

UINT_PTR base; 

class Color {
public:
	float red, green, blue, alpha;
	Color(float r, float g, float b, float a) {
		red = r;
		green = g;
		blue = b;
		alpha = a;
	}
};

enum Status {
	SUCCESS,
	FAILED_PARSING,
	FAILED_PARSING_MOD_INFO,
	FAILED_LOADING
};

class Mod {
public:
	std::string name, version, author, description, path;
	Status status;
	std::vector<std::string> dlls;
	Mod(std::string name, std::string version, std::string author, std::string description, std::vector<std::string> dlls, std::string path, Status status) {
		this->name = name;
		this->version = version;
		this->author = author;
		this->description = description;
		this->dlls = dlls;
		this->path = path;
		this->status = status;
	}

	Mod(std::string path, Status status) {
		std::vector<std::string> empty;
		this->name = std::string("");
		this->version = std::string("");
		this->author = std::string("");
		this->description = std::string("");
		this->dlls = empty;
		this->path = path;
		this->status = status;
	}
};

std::vector <Mod> mods;

Color defaultColor = Color(1.0, 1.0, 1.0, 1.0);
DWORD defaultColorPtr = (DWORD)&defaultColor;

wchar_t defaultMessage[1024];
DWORD defaultMessagePtr = (DWORD)&defaultMessage;

char msgObject[255];
DWORD msgObjectPtr = (DWORD)&msgObject;

void DLL_EXPORT ASMPrintMessage() {
	asm("push [_defaultMessagePtr]");
	asm("mov ecx, [_msgObjectPtr]");

	asm("mov eax, [_base]");
	asm("add eax, 0x0EB60");
	asm("call eax"); //call some message constructing function

	asm("mov ecx, [_base]");
	asm("add ecx, 0x36B1C8");
	asm("mov ecx, [ecx]"); //ecx points to gamecontroller
	asm("mov ecx, [ecx + 0x800A14]"); //ecx points to chatwidget

	asm("push [_defaultColorPtr]");
	asm("push [_msgObjectPtr]");
	asm("mov edx, [_base]");
	asm("add edx, 0x3AB30");
	asm("call edx"); //prints message


	asm("mov ecx, [_msgObjectPtr]");

	asm("mov eax, [_base]");
	asm("add eax, 0x193E50");
	asm("call eax"); //destructor for that message object
}

void DLL_EXPORT PrintMessage(wchar_t message[]) {
	wcsncpy(defaultMessage, message, 255);
	defaultColor.red = 1.0;
	defaultColor.blue = 1.0;
	defaultColor.green = 1.0;
	defaultColor.alpha = 1.0;
	ASMPrintMessage();
}

void DLL_EXPORT PrintMessage(wchar_t message[], int r, int g, int b) {
	wcsncpy(defaultMessage, message, 255);
	defaultColor.red = r / 255.0;
	defaultColor.green = g / 255.0;
	defaultColor.blue = b / 255.0;
	ASMPrintMessage();
}

void ModsMessage(wchar_t message[]) {
	PrintMessage((wchar_t*)L"[", 230, 100, 0);
	PrintMessage((wchar_t*)L"Mods", 255, 140, 0);
	PrintMessage((wchar_t*)L"] ", 230, 100, 0);
	PrintMessage(message);
}

void HighlightedMessage(wchar_t highlighted[], wchar_t details[]) {
	PrintMessage(highlighted, 245, 215, 65);
	PrintMessage(details);
}

void HighlightedMessage(wchar_t highlighted[], wchar_t details[], int r, int g, int b) {
	PrintMessage(highlighted, r, g, b);
	PrintMessage(details);
}

bool FileExists(LPCTSTR szPath) {
  DWORD dwAttrib = GetFileAttributes(szPath);
  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

std::string GetCurrentWorkingDir(void) {
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	return current_working_dir;
}

std::vector<std::string> findDirectories(const char filepath[]) {
	std::vector <std::string> directories;

	//Find directories
	HANDLE hFind;
	WIN32_FIND_DATA data;

	hFind = FindFirstFile(filepath, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				directories.push_back(data.cFileName);
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	directories.erase(directories.begin(), directories.begin() + 2);

	return directories;
}

std::vector<std::string> findFiles(const char filepath[]) {
	std::vector <std::string> files;

	//Find files
	HANDLE hFind;
	WIN32_FIND_DATA data;

	hFind = FindFirstFile(filepath, &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			files.push_back(data.cFileName);
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

	return files;
}

rapidjson::Document parseJSONFromFile(const char filepath[]) {
	FILE* fp = fopen(filepath, "rb"); // non-Windows use "r"
	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	rapidjson::Document document;
	document.ParseStream(is);
	fclose(fp);

	return document;
}

void loadDLL(Mod mod, const char* dllname) {
	LoadLibraryA((GetCurrentWorkingDir() + std::string("\\Mods\\") + mod.path + std::string("\\") + std::string(dllname)).c_str());
}

void __stdcall DLL_EXPORT loadMods() {
	const char MOD_PATH[] = "Mods\\*";

	for (std::string S_DIRName : findDirectories(MOD_PATH)) {
		std::string MOD_INFO_PATH = std::string("Mods\\") + S_DIRName + std::string("\\mod");

		if (FileExists(MOD_INFO_PATH.c_str())) {
			try {
				rapidjson::Document mod_info = parseJSONFromFile(MOD_INFO_PATH.c_str());
				
				if (mod_info.HasParseError()) {
					Mod* mod = new Mod(
						S_DIRName,
						FAILED_PARSING_MOD_INFO
					);
					mods.push_back(*mod);
					continue;
				}

				Mod* mod = new Mod(
					mod_info.HasMember("name") ? std::string(mod_info["name"].GetString()) : NULL,
					mod_info.HasMember("version") ? std::string(mod_info["version"].GetString()) : NULL,
					mod_info.HasMember("author") ? std::string(mod_info["author"].GetString()) : NULL,
					mod_info.HasMember("description") ? std::string(mod_info["description"].GetString()) : std::string(""),
					findFiles((std::string("Mods\\") + S_DIRName + std::string("\\*.dll")).c_str()),
					S_DIRName,
					SUCCESS
				);
				mods.push_back(*mod);

			}
			catch (std::exception& e) {
				Mod* mod = new Mod(
					S_DIRName,
					FAILED_PARSING_MOD_INFO
				);
				mods.push_back(*mod);
			}
		}
	}

	//Inject DLLs
	for (Mod mod : mods) {
		try {
			for (std::string dll : mod.dlls) {
				loadDLL(mod, dll.c_str());				
			}
		}
		catch (std::exception& e) {
			mod.status = FAILED_LOADING;
		}
	}
}

std::wstring getWC(std::string name) {
	return std::wstring(name.begin(), name.end());
}

std::wstring getWCRawFilename(std::string filename) {
	size_t lastindex = filename.find_last_of(".");
	std::string rawname = filename.substr(0, lastindex);
	rawname.append("\n");
	return getWC(rawname);
}

std::wstring getShortModString(Mod mod) {
	std::string str = mod.name + " (" + mod.version + ") by " + mod.author + "\n";
	return getWC(str);
}

bool __stdcall DLL_EXPORT HandleMessage(wchar_t msg[], unsigned int msg_size) {
	//Display commands
	if (!wcscmp(msg, L"/mods")) {
		ModsMessage((wchar_t*)L":\n");
	
		for (Mod mod : mods) {
			switch (mod.status) {
				case SUCCESS:
					HighlightedMessage((wchar_t*)L"Success: ", (wchar_t*)getShortModString(mod).c_str(), 0, 255, 0);
					break;

				case FAILED_PARSING:
					HighlightedMessage((wchar_t*)L"Failed Parsing: ", (wchar_t*)getWCRawFilename(mod.path).c_str(), 255, 0, 0);
					break;

				case FAILED_PARSING_MOD_INFO:
					HighlightedMessage((wchar_t*)L"Failed Parsing ModInfo: ", (wchar_t*)getWCRawFilename(mod.path).c_str(), 255, 0, 0);
					break;

				case FAILED_LOADING:
					HighlightedMessage((wchar_t*)L"Failed Loading: ", (wchar_t*)getWCRawFilename(mod.path).c_str(), 255, 0, 0);
					break;
			}
		}

		PrintMessage((wchar_t*)L"\nSpecial Thanks to: ");
		HighlightedMessage((wchar_t*)L"ChrisMiuchiz", (wchar_t*)L"\n");

		return true;
	}
	return false;
}
DWORD HandleMessagePtr = (DWORD)&HandleMessage;

typedef bool(__stdcall *ChatEventCallback)(wchar_t buf[], unsigned int msg_size);
typedef void(*RegisterChatEventCallback_t)(ChatEventCallback cb);
DWORD WINAPI DLL_EXPORT RegisterCallbacks() {
	HMODULE modManagerDLL = LoadLibraryA("CallbackManager.dll");
	auto RegisterChatEventCallback = (RegisterChatEventCallback_t)GetProcAddress(modManagerDLL, "RegisterChatEventCallback");
	RegisterChatEventCallback((ChatEventCallback)HandleMessage);
	return 0;
}

extern "C" DLL_EXPORT BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	base = (UINT_PTR) GetModuleHandle(NULL);

	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)loadMods, 0, 0, NULL);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RegisterCallbacks, 0, 0, NULL);
		break;
	}
	return TRUE;
}