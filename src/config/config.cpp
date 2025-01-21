// Config.cpp
#include "config.h"
#include <fstream>
#include <algorithm>
#include <iostream>

// Initialize the static shared_ptr
std::shared_ptr<std::map<std::string, std::string>> Config::settings = nullptr;

std::string Config::get(const std::string& key) {
    if (!settings) {
        settings = loadConfig();  // Lazy load the configuration
    }
    if (settings->find(key) != settings->end()) {
        return (*settings)[key];
    }
    return "";  // Return empty if the key is not found
}

std::shared_ptr<std::map<std::string, std::string>> Config::loadConfig() {
    auto config = std::make_shared<std::map<std::string, std::string>>();
    std::ifstream file("config.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open config.txt\n";
        return config;  // Return empty map if file can't be opened
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;  // Invalid line, skip
        }

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        (*config)[key] = value;
    }

    file.close();
    return config;
}

std::string Config::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
}
