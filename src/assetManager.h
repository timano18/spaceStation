#pragma once


#include <iostream>
#include <filesystem>
#include <vector>
#include <format>


namespace fs = std::filesystem;


std::unordered_map<std::string, fs::path> findModelFiles(const char* dir);

void watchFile(const char* path);

std::filesystem::file_time_type getWriteTime(const char* path);
