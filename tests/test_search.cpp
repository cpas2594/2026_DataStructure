#include "search/SearchEngine.h"

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <string>
#include "structures/DynamicArray.hpp"

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

void addToIndexes(const DynamicArray<Hospital>& hospitals,
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
            CHECK(byName.insert(name, DynamicArray<std::string>{h.id}));
        }
    }
}

int main() {
    DynamicArray<Hospital> hospitals = {
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

    const DynamicArray<Hospital> none;
    HospitalIdIndex noneIds;
    HospitalNameIndex noneNames;
    SearchEngine emptyEngine(none, noneIds, noneNames);
    CHECK(emptyEngine.search("").empty());
    CHECK(emptyEngine.search("서울").empty());

    CHECK(engine.searchDetailed("").matchType == MatchType::All);
    CHECK(engine.searchDetailed("A1100002").matchType == MatchType::ExactId);
    CHECK(engine.searchDetailed("서울중앙병원").matchType == MatchType::ExactName);
    CHECK(engine.searchDetailed("중앙").matchType == MatchType::PartialName);
    CHECK(engine.searchDetailed("없는병원").matchType == MatchType::PartialName);
    CHECK(engine.searchDetailed("없는병원").hospitals.empty());

    result = engine.search(" a1100002 ");
    CHECK(result.size() == 1 && result[0].id == "A1100002");
    CHECK(SearchEngine::normalizeId(" a1100002\t") == "A1100002");
    CHECK(SearchEngine::normalizeId("가a1") == "가A1");

    {
        const DynamicArray<Hospital> raw = {
            makeHospital(" a1 ", "  가나병원 "),
            makeHospital("A1", "다른병원"),
            makeHospital("", "HPID없음"),
            makeHospital("A2", "가나병원"),
            makeHospital("A3", "가나병원 별관"),
        };
        HospitalIdIndex ids;
        HospitalNameIndex names;
        DynamicArray<Hospital> stored;
        for (const Hospital& h : raw) {
            if (SearchEngine::addToIndexes(h, stored.size(), ids, names)) {
                stored.push_back(h);
            }
        }
        CHECK(stored.size() == 3);
        CHECK(ids.size() == 3);
        CHECK(names.find("가나병원") != nullptr && names.find("가나병원")->size() == 2);
        CHECK(names.find("다른병원") == nullptr);

        SearchEngine e(stored, ids, names);
        auto r = e.searchDetailed("가나병원");
        CHECK(r.matchType == MatchType::ExactName);
        CHECK(r.hospitals.size() == 2);
        r = e.searchDetailed("A1");
        CHECK(r.matchType == MatchType::ExactId && r.hospitals.size() == 1);
        r = e.searchDetailed("별관");
        CHECK(r.matchType == MatchType::PartialName && r.hospitals.size() == 1);
    }

    {
        const DynamicArray<Hospital> tricky = {
            makeHospital("B100", "병원B"),
            makeHospital("B200", "B100"),
        };
        HospitalIdIndex ids;
        HospitalNameIndex names;
        for (std::size_t i = 0; i < tricky.size(); ++i) {
            SearchEngine::addToIndexes(tricky[i], i, ids, names);
        }
        SearchEngine e(tricky, ids, names);
        auto r = e.searchDetailed("B100");
        CHECK(r.matchType == MatchType::ExactId);
        CHECK(r.hospitals.size() == 1 && r.hospitals[0].id == "B100");
    }

    std::cout << "[PASS] test_search: HPID, exact, duplicate names, partial, empty, missing, match type, HPID case, index rules, priority\n";
    return 0;
}
