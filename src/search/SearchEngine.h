#pragma once
#include "structures/BasicTypes.hpp"

#include "model/Hospital.h"
#include "structures/HashTable.hpp"


using HospitalIdIndex = HashTable<size_t>;
using HospitalNameIndex = HashTable<SearchArray<Text>>;

enum class MatchType {
    All,
    ExactId,
    ExactName,
    PartialName,
};

struct SearchResult {
    MatchType matchType = MatchType::All;
    SearchArray<Hospital> hospitals;
};

class SearchEngine {
public:
    SearchEngine(const SearchArray<Hospital>& hospitals,
                 const HospitalIdIndex& byId,
                 const HospitalNameIndex& byName);

    SearchResult searchDetailed(const Text& input) const;

    SearchArray<Hospital> search(const Text& input) const;

    static Text trim(const Text& input);

    static Text normalizeName(const Text& name);

    static Text normalizeId(const Text& id);

    static bool addToIndexes(const Hospital& hospital, size_t index,
                             HospitalIdIndex& byId, HospitalNameIndex& byName);

private:
    const SearchArray<Hospital>& hospitals_;
    const HospitalIdIndex& byId_;
    const HospitalNameIndex& byName_;
};
