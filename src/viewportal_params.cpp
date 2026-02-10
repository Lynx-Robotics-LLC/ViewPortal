#include "viewportal_params.h"
#include <fstream>
#include <sstream>

namespace viewportal {

namespace {

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end == std::string::npos ? std::string::npos : end - start + 1);
}

bool parseLine(const std::string& line, std::string& key, std::string& value) {
    size_t comment = line.find('#');
    std::string stripped = comment == std::string::npos ? line : line.substr(0, comment);
    size_t eq = stripped.find('=');
    if (eq == std::string::npos) return false;
    key = trim(stripped.substr(0, eq));
    value = trim(stripped.substr(eq + 1));
    return !key.empty();
}

template<typename T>
bool parseInteger(const std::string& s, T& out) {
    try {
        size_t pos = 0;
        int val = std::stoi(s, &pos);
        if (pos != s.size()) return false;
        out = static_cast<T>(val);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

LoadedParams loadParams(const std::string& path) {
    LoadedParams result;
    result.viewportal = ViewPortalParams();
    result.window_title_storage = "ViewPortal";
    result.viewportal.window_title = result.window_title_storage.c_str();

    std::ifstream f(path);
    if (!f.is_open()) return result;

    std::string line;
    while (std::getline(f, line)) {
        std::string key, value;
        if (!parseLine(line, key, value)) continue;

        if (key == "window_width") {
            parseInteger(value, result.viewportal.window_width);
        } else if (key == "window_height") {
            parseInteger(value, result.viewportal.window_height);
        } else if (key == "panel_width") {
            parseInteger(value, result.viewportal.panel_width);
        }
    }

    result.viewportal.window_title = result.window_title_storage.c_str();
    return result;
}

} // namespace viewportal
