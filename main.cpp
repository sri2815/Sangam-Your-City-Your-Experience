// Module 1 backend: Place Search & Autocomplete
// Data structures: Trie (prefix search) + Hash Table (fast place lookup)
//
// Requires the single-header library cpp-httplib:
//   https://github.com/yhirose/cpp-httplib  (download httplib.h into this folder)
//
// Build:  g++ -std=c++17 -O2 main.cpp -o server -pthread
// Run:    ./server            (then open index.html in a browser)

#include <fstream>
#include <iostream>
#include <sstream>

#include "httplib.h"
#include "place_table.h"
#include "trie.h"

static std::string jsonEscape(const std::string &s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            default: out += c;
        }
    }
    return out;
}

static std::string placeToJson(const Place &p) {
    std::ostringstream ss;
    ss << "{\"name\":\"" << jsonEscape(p.name) << "\","
       << "\"lat\":" << p.lat << ",\"lng\":" << p.lng << ","
       << "\"category\":\"" << jsonEscape(p.category) << "\"}";
    return ss.str();
}

static bool loadPlaces(const std::string &path, Trie &trie, PlaceTable &table) {
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    std::getline(f, line);  // skip header
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string name, lat, lng, cat;
        std::getline(ss, name, ',');
        std::getline(ss, lat, ',');
        std::getline(ss, lng, ',');
        std::getline(ss, cat, ',');
        if (!cat.empty() && cat.back() == '\r') cat.pop_back();
        Place p{name, std::stod(lat), std::stod(lng), cat};
        trie.insert(p.name);
        table.insert(p);
    }
    return true;
}

int main() {
    Trie trie;
    PlaceTable table;

    if (!loadPlaces("places.csv", trie, table)) {
        std::cerr << "Could not open places.csv\n";
        return 1;
    }
    std::cout << "Loaded " << table.size() << " places\n";

    httplib::Server svr;

    // Allow the browser frontend to call us from any origin / file://
    svr.set_default_headers({{"Access-Control-Allow-Origin", "*"}});

    // GET /search?q=rob  -> autocomplete suggestions with full place data
    svr.Get("/search", [&](const httplib::Request &req, httplib::Response &res) {
        std::string q = req.has_param("q") ? req.get_param_value("q") : "";
        std::string json = "[";
        if (!q.empty()) {
            bool first = true;
            for (const auto &name : trie.autocomplete(q, 8)) {
                const Place *p = table.find(name);  // hash table lookup
                if (!p) continue;
                if (!first) json += ",";
                json += placeToJson(*p);
                first = false;
            }
        }
        json += "]";
        res.set_content(json, "application/json");
    });

    // GET /place?name=Clock Tower  -> exact lookup
    svr.Get("/place", [&](const httplib::Request &req, httplib::Response &res) {
        const Place *p = table.find(req.get_param_value("name"));
        if (!p) {
            res.status = 404;
            res.set_content("{\"error\":\"not found\"}", "application/json");
            return;
        }
        res.set_content(placeToJson(*p), "application/json");
    });

    // GET /places -> every place (used to drop markers on the map at load)
    svr.Get("/places", [&](const httplib::Request &, httplib::Response &res) {
        std::string json = "[";
        bool first = true;
        for (const auto &p : table.all()) {
            if (!first) json += ",";
            json += placeToJson(p);
            first = false;
        }
        json += "]";
        res.set_content(json, "application/json");
    });

    std::cout << "Server running at http://localhost:8080\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
