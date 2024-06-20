#include "pch.h"
#include "assetManager.h"

std::unordered_map<std::string, fs::path> findModelFiles(const char* dir)
{
	std::unordered_map<std::string, fs::path> gltf_files;
	fs::path directorypath = dir;
	try
	{
		if (exists(directorypath) && fs::is_directory(directorypath))
		{
			for (const auto& entry :
				fs::recursive_directory_iterator(directorypath))
			{
				if (entry.is_regular_file() && entry.path().extension() == ".gltf")
				{
					std::string filename = entry.path().filename().string();
					gltf_files[filename] = entry.path();
					//std::cout << filename << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "Directory not found." << std::endl;
		}
	}
	catch (const fs::filesystem_error& e)
	{
		std::cerr << "Filesystem error: " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "General error: " << e.what() << std::endl;
	}
	return gltf_files;

}
