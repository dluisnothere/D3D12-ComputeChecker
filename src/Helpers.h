#pragma once
#include <string>
#include <fstream>
#include <Windows.h>
#include <string>

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

// DEBUG helper function to write log outputs
inline void WriteLog(const std::string& message) {
	std::ofstream logFile("trace_log.txt", std::ios_base::app); // Open in append mode
	if (logFile.is_open()) {
		logFile << message << std::endl;
		logFile.close();
	}
}

inline std::string WStringToString(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

inline std::string GetCurrentWorkingDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}