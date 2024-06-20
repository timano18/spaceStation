#pragma once


#include <iostream>
#include <filesystem>
#include <vector>


namespace fs = std::filesystem;


std::unordered_map<std::string, fs::path> findModelFiles(const char* dir);


