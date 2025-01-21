// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>
#include <memory>

class Config {
public:
    static std::string get(const std::string& key);

private:
    static std::shared_ptr<std::map<std::string, std::string>> settings;
    static std::shared_ptr<std::map<std::string, std::string>> loadConfig();
    static std::string trim(const std::string& str);
};

#endif // CONFIG_H
