#pragma once
#include <windows.h> 
#include <iostream>
#include <streambuf>
#include <string>
#include <sstream>
#include "DLLConfig.hpp"

class CC_API Streambuf : public std::streambuf {
private: 
	std::ostringstream buffer;

protected:
	enum class LogType {
		Log,
		LogWarning,
		LogError,
		LogImportant
	};

	static HANDLE hConsole;

	static void SetMode(LogType logType) {
		if (hConsole == NULL) {
			hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		}

		if (logType == LogType::LogWarning) {
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
		}
		else if (logType == LogType::LogError) {
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
		}
		else if (logType == LogType::LogImportant) {
			SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
		}
		else {
			SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		}
	}

	virtual std::streamsize xsputn(const char* s, std::streamsize n) override {
		for (std::streamsize i = 0; i < n; ++i) {
			// Buffer each character
			buffer << s[i];

			// When a newline is encountered, prefix and flush the buffer
			if (s[i] == '\n') {
				std::string str = "[CC]: " + buffer.str();
				std::cerr.write(str.c_str(), str.size());
				buffer.str(""); 
				buffer.clear();
			}
		}
		return n;
	}

	virtual int overflow(int c = EOF) override {
		if (c != EOF) {
			buffer << static_cast<char>(c);
			if (c == '\n') {
				std::string str = "[CC]: " + buffer.str();
				std::cerr.write(str.c_str(), str.size());
				buffer.str(""); 
				buffer.clear();
			}
		}
		return c;
	}

public:
	static void Log(std::string str) {
		SetMode(LogType::Log);
		std::cerr << str << "\n";
	}

	static void Warning(std::string str) {
		SetMode(LogType::LogWarning);
		std::cerr << str << "\n";
		SetMode(LogType::Log);
	}

	static void Error(std::string str) {
		SetMode(LogType::LogError);
		std::cerr << str << "\n";
		SetMode(LogType::Log);
	}

	static void Important(std::string str) {
		SetMode(LogType::LogImportant);
		std::cerr << str << "\n";
		SetMode(LogType::Log);
	}
};