#pragma once

#include "model/Hospital.h"
#include "structures/HashTable.hpp"
#include <cstddef>
#include <string>
#include "structures/DynamicArray.hpp"

using HospitalIdIndex = HashTable<std::size_t>;
using HospitalNameIndex = HashTable<DynamicArray<std::string>>;

enum class MatchType {
    All,
    ExactId,
    ExactName,
    PartialName,
};

struct SearchResult {
    MatchType matchType = MatchType::All;
    DynamicArray<Hospital> hospitals;
};

class SearchEngine {
public:
    SearchEngine(const DynamicArray<Hospital>& hospitals,
                 const HospitalIdIndex& byId,
                 const HospitalNameIndex& byName);

    SearchResult searchDetailed(const std::string& input) const;

    DynamicArray<Hospital> search(const std::string& input) const;

    static std::string trim(const std::string& input);

    static std::string normalizeName(const std::string& name);

    static std::string normalizeId(const std::string& id);

    static bool addToIndexes(const Hospital& hospital, std::size_t index,
                             HospitalIdIndex& byId, HospitalNameIndex& byName);

private:
    const DynamicArray<Hospital>& hospitals_;
    const HospitalIdIndex& byId_;
    const HospitalNameIndex& byName_;
};
