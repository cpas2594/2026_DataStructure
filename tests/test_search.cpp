#include "search/SearchEngine.h"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
void requireTrue(bool ok, const char* expression, const char* file, int line) {
    if (!ok) {
        std::ostringstream message;
        message << file << ':' << line << " CHECK failed: " << expression;
        throw std::runtime_error(message.str());
    }
}
}
#define CHECK(expression) requireTrue(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
Hospital makeHospital(const std::string& id, const std::string& name) {
    Hospital h{};
    h.id = id;
    h.name = name;
    return h;
}

void addToIndexes(const std::vector<Hospital>& hospitals,
                  HospitalIdIndex& byId,
                  HospitalNameIndex& byName) {
    for (std::size_t i = 0; i < hospitals.size(); ++i) {
        const Hospital& h = hospitals[i];

        if (!byId.insert(h.id, i)) {
            continue;
        }
        const std::string name = SearchEngine::trim(h.name);
        if (auto* ids = byName.find(name)) {
            ids->push_back(h.id);
        } else {
            CHECK(byName.insert(name, std::vector<std::string>{h.id}));
        }
    }
}

int main() {
    std::vector<Hospital> hospitals = {
        makeHospital("A1100001", "서울중앙병원"),
        makeHospital("A1100002", "한강병원"),
        makeHospital("A1100004", "늘봄병원"),
        makeHospital("A1100007", "서울중앙병원"),
        makeHospital("A1100010", "강남응급센터")
    };

    HospitalIdIndex byId;
    HospitalNameIndex byName;
    addToIndexes(hospitals, byId, byName);
    SearchEngine engine(hospitals, byId, byName);

    CHECK(SearchEngine::trim(" \t 서울중앙병원  \n") == "서울중앙병원");
    CHECK(SearchEngine::trim(" \t\r\n").empty());
    CHECK(engine.search("  ").size() == 5);

    auto result = engine.search("\tA1100002 \n");
    CHECK(result.size() == 1);
    CHECK(result[0].id == "A1100002");

    result = engine.search(" 서울중앙병원 ");
    CHECK(result.size() == 2);
    CHECK(result[0].id == "A1100001");
    CHECK(result[1].id == "A1100007");

    result = engine.search("병원");
    CHECK(result.size() == 4);
    result = engine.search("중앙");
    CHECK(result.size() == 2);
    result = engine.search("응급");
    CHECK(result.size() == 1 && result[0].id == "A1100010");

    CHECK(engine.search("존재하지않는병원").empty());
    CHECK(engine.search("A0000000").empty());

    const std::vector<Hospital> none;
    HospitalIdIndex noneIds;
    HospitalNameIndex noneNames;
    SearchEngine emptyEngine(none, noneIds, noneNames);
    CHECK(emptyEngine.search("").empty());
    CHECK(emptyEngine.search("서울").empty());

    std::cout << "[PASS] test_search: HPID, exact, duplicate names, partial, empty, missing\n";
    return 0;
}
