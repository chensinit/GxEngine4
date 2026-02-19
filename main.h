#ifndef MAIN_H
#define MAIN_H

#include "src/scene.h"
#include "src/resource/resourceManager.h"
#include <string>
#include <chrono>

bool loadSettings(const std::string &filePath, int &width, int &height, std::string &resourceFile);

#endif // MAIN_H
