#define _CRT_SECURE_NO_WARNINGS // _wcslwr()
#include <atlconv.h>
#include <atlbase.h>
#include <binaryhandler.hpp>
#include <filesystem>
#include <fstream>
#include <gzip/compress.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <iostream>
#include <json.hpp>
#include <regex>
#include <string>
#include <vector>
#include <windows.h>
using dataEncode = nlohmann::json;
std::wstring defaultFile = L"OBSoptions.bk";
std::wstring bkFile = defaultFile;
std::wstring unexpandedPath = L"%appdata%\\obs-studio";
std::wstring obsPath;
dataEncode bkJSON;

/*
MIT License

Copyright (c) 2022 nk125

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

inline std::string wstrToStr(std::wstring wstr) {
	return std::string(ATL::CW2A(wstr.data()).m_psz);
}

inline std::wstring strToWstr(std::string str) {
	return std::wstring(ATL::CA2W(str.data()).m_psz);
}

void writeToBackup(std::wstring fPath) {
	std::wstring fileP = fPath;
	std::wstring rOBS;

	rOBS = std::regex_replace(obsPath, std::wregex(LR"(\\)"), L"\\\\");
	fileP = std::regex_replace(fileP, std::wregex(rOBS), L"");
	std::string ffPath = wstrToStr(fileP);

	nk125::binary_file_handler fOperator;
	std::vector<unsigned char> mpJSON, compF;

	try {
		std::string con = fOperator.read_file(wstrToStr(fPath));
		std::string cmp = gzip::compress(con.c_str(), con.size());
		compF.assign(cmp.begin(), cmp.end());
		bkJSON["files"][ffPath] = dataEncode(compF);
		mpJSON = dataEncode::to_msgpack(bkJSON);

		fOperator.write_file(wstrToStr(bkFile), std::string(mpJSON.begin(), mpJSON.end()));
	}
	catch (...) {
		std::cerr << "There was an error writing the backup file\n";
		std::exit(2);
	}
}

void saveConfig(std::filesystem::directory_iterator obsDir) {
	bkJSON["files"] = {};

	std::vector<std::wstring> confPaths = {
		obsPath + L"\\plugin_config",
		obsPath + L"\\basic"
	};

	for (std::filesystem::directory_entry dIt : obsDir) {
		std::wstring fPath = dIt.path().wstring();

		if (dIt.is_directory()) {
			if (std::find(confPaths.begin(), confPaths.end(), fPath) != confPaths.end()) {
				for (std::filesystem::directory_entry itFile : std::filesystem::recursive_directory_iterator(fPath)) {
					if (itFile.is_regular_file()) {
						writeToBackup(itFile.path().wstring());
					}
				}
			}
		}
		else {
			if (fPath == obsPath + L"\\global.ini") {
				writeToBackup(fPath);
			}
		}
	}

	std::wcout << "Config Saved in " << bkFile << "!\n";
}

void loadConfig() {
	nk125::binary_file_handler fOperator;
	std::string fCon;

	try {
		fCon = fOperator.read_file(wstrToStr(bkFile));
	}
	catch (...) {
		std::cerr << "The OBS backup config file doesn't exist or require admin perms to read\n";
		std::exit(2);
	}
	dataEncode dsData;

	try {
		dsData = dataEncode::from_msgpack(fCon);
	}
	catch (...) {
		std::cerr << "The OBS backup config file have invalid/corrupted data\n";
		std::exit(2);
	}

	for (auto& kv : dsData["files"].items()) {
		std::vector<unsigned char> value = kv.value();
		std::string decomp(value.begin(), value.end());

		std::wstring pathToWrite = obsPath + strToWstr(kv.key());

		if (!gzip::is_compressed(decomp.c_str(), decomp.size())) {
			std::cerr << "The OBS backup config file have invalid/corrupted data\n";
			std::exit(2);
		}

		decomp = gzip::decompress(decomp.c_str(), decomp.size());

		std::wstring folderToWrite = pathToWrite.substr(0, pathToWrite.find_last_of(L"\\"));
		std::filesystem::create_directories(folderToWrite);

		fOperator.write_file(wstrToStr(pathToWrite), decomp);
	}

	std::wcout << "Config was loaded succesfully from " << bkFile << "!\n";
}

inline std::wstring toLowerCase(std::wstring str) {
	return std::wstring(_wcslwr(str.data()));
}

void usage() {
	std::wcout << "Usage:\n\n"
		<< "  Argument 1: Save/Load - Operation Mode\n"
		<< "  Argument 2: Path to save/load config file, by default this is " << defaultFile << "\n\n";
}

int wmain(int argc, wchar_t* argv[]) {
	if (argc <= 1) {
		usage();
		return 1;
	}

	// Is assumed that there's at least 2 arguments

	std::filesystem::directory_iterator obsDir;
	obsPath.resize(MAX_PATH);

	if (ExpandEnvironmentStringsW(unexpandedPath.data(), &obsPath[0], MAX_PATH) == 0) {
		std::cerr << "There was an error expanding OBS path.\n";
		return 1;
	}

	obsPath = std::regex_replace(obsPath, std::wregex(LR"(\0)"), L"");

	try {
		obsDir = std::filesystem::directory_iterator(obsPath);
	}
	catch (std::filesystem::filesystem_error& fsErr) {
		std::wcerr << "OBS isn't installed or admin perms are required.\n" << fsErr.what() << "\n";
		return 1;
	}

	if (argc >= 3) {
		bkFile.assign(argv[2]);
	}

	std::wstring mode(argv[1]);
	mode = toLowerCase(mode);

	if (mode == L"save") {
		saveConfig(obsDir);
	}
	else if (mode == L"load") {
		loadConfig();
	}
	else {
		usage();
		return 1;
	}
}