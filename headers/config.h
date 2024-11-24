#ifndef CONFIG_H
#define CONFIG_H

#include <string>

bool fileExists(const std::string& filename);
void createDefaultConfig(const std::string& filename);
bool readRunInTray(const std::string& filename);
std::string readRobloxCookie(const std::string& filename);
std::string readKeybind(const std::string& filename);

#endif
