#include "../headers/config.h"
#include "../headers/globals.h"
#include <fstream>
#include <iostream>
#include <json/json.h>

bool fileExists(const std::string& filename) {
    std::ifstream file(filename);
    return file.good();
}

void createDefaultConfig(const std::string& filename) {
    Json::Value config;
    config["ROBLOX_COOKIE"] = "";
    config["KEYBIND"] = "F3";
    config["RUN_IN_TRAY"] = false;

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    std::string configContent = Json::writeString(writer, config);

    std::ofstream configFile(filename);
    if (!configFile.is_open()) {
        std::cout << "Failed to create " << filename << ". Please check permissions.\n";
        std::cout << "Press Enter to exit...\n";
        std::cin.get();
        return;
    }

    configFile << configContent;
    configFile.close();

    std::cout << filename << " has been created. Please add your ROBLOX_COOKIE and run the program again.\n";
    std::cout << "Press Enter to exit...\n";
    std::cin.get();
}

bool readRunInTray(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::cout << "Failed to open " << filename << ".\n";
        return false;
    }

    Json::Value config;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    if (!Json::parseFromStream(readerBuilder, configFile, &config, &errs)) {
        return false;
    }

    return config.get("RUN_IN_TRAY", false).asBool();
}

std::string readRobloxCookie(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::cout << "Failed to open " << filename << ".\n";
        return "";
    }

    Json::Value config;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    if (!Json::parseFromStream(readerBuilder, configFile, &config, &errs)) {
        return "";
    }

    if (!config.isMember("ROBLOX_COOKIE")) {
        std::cout << "ROBLOX_COOKIE not found in " << filename << ".\n";
        return "";
    }

    std::string cookie = config["ROBLOX_COOKIE"].asString();
    if (cookie.empty()) {
        std::cout << "ROBLOX_COOKIE is empty in " << filename << ".\n";
    }

    return cookie;
}

std::string readKeybind(const std::string& filename) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::cout << "Failed to open " << filename << ".\n";
        return "";
    }

    Json::Value config;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    if (!Json::parseFromStream(readerBuilder, configFile, &config, &errs)) {
        return "";
    }

    if (!config.isMember("KEYBIND")) {
        std::cout << "KEYBIND not found in " << filename << ".\n";
        return "";
    }

    std::string keybind = config["KEYBIND"].asString();
    if (keybind.empty()) {
        std::cout << "KEYBIND is empty in " << filename << ".\n";
    }

    return keybind;
}
