#include "pch.h"
#include "cosmic.h"

#include "assetManager.h"
#include <thread>

int main(int argc, char* argv[])
{
	std::string filePath = "C:/spaceStationProjectDirectory/changeTest/ABeautifulGame/glTF/ABeautifulGame.gltf";

	std::thread watcher(watchFile, filePath.c_str());
	watcher.detach();
	run();

	return 0;
}








