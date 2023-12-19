#include <fstream>
#include <iostream>
#include <jsonlib/single_include/nlohmann/json.hpp>
#include <stdexcept>

nlohmann::basic_json<> ReadConfig() {
  std::ifstream file("config.json");
  if (!file.is_open()) {
    throw std::runtime_error("Open config.json failed");
    return 0;
  }
  nlohmann::basic_json<> json = nlohmann::json::parse(file);
  return json;
}