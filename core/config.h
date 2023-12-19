#ifndef CONFIG
#define CONFIG

#include <jsonlib/single_include/nlohmann/json.hpp>
nlohmann::basic_json<> ReadConfig();

#endif