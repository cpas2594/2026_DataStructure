#include "structures/BasicTypes.hpp"
#include "search/SearchEngine.h"


SearchEngine::SearchEngine(const SearchArray<Hospital>& hospitals,
                           const HospitalIdIndex& byId,
                           const HospitalNameIndex& byName)
    : hospitals_(hospitals), byId_(byId), byName_(byName) {}

Text SearchEngine::trim(const Text& input) {
    size_t start = 0;
    while (start < input.size() &&
           isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    size_t end = input.size();
    while (end > start &&
           isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

Text SearchEngine::normalizeName(const Text& name) {
    return trim(name);
}

Text SearchEngine::normalizeId(const Text& id) {
    Text result = trim(id);
    for (char& c : result) {
        const unsigned char byte = static_cast<unsigned char>(c);
        if (byte < 0x80) {
            c = static_cast<char>(toupper(byte));
        }
    }
    return result;
}

bool SearchEngine::addToIndexes(const Hospital& hospital, size_t index,
                                HospitalIdIndex& byId, HospitalNameIndex& byName) {
    const Text id = normalizeId(hospital.id);
    if (id.empty() || !byId.insert(id, index)) {
        return false;
    }
    const Text name = normalizeName(hospital.name);
    if (name.empty()) {
        return true;
    }
    if (SearchArray<Text>* ids = byName.find(name)) {
        ids->push_back(id);
    } else {
        byName.insert(name, SearchArray<Text>{id});
    }
    return true;
}

SearchResult SearchEngine::searchDetailed(const Text& input) const {
    SearchResult result;
    const Text key = normalizeName(input);

    if (key.empty()) {
        result.matchType = MatchType::All;
        result.hospitals = hospitals_;
        return result;
    }

    const Text idKey = normalizeId(key);
    if (const size_t* index = byId_.find(idKey)) {
        if (*index < hospitals_.size() &&
            normalizeId(hospitals_[*index].id) == idKey) {
            result.matchType = MatchType::ExactId;
            result.hospitals.push_back(hospitals_[*index]);
            return result;
        }
    }

    if (const SearchArray<Text>* ids = byName_.find(key)) {
        result.hospitals.reserve(ids->size());
        for (const Text& id : *ids) {
            const size_t* index = byId_.find(id);
            if (index != nullptr && *index < hospitals_.size() &&
                normalizeName(hospitals_[*index].name) == key) {
                result.hospitals.push_back(hospitals_[*index]);
            }
        }
        if (!result.hospitals.empty()) {
            result.matchType = MatchType::ExactName;
            return result;
        }
    }

    result.matchType = MatchType::PartialName;
    for (const Hospital& hospital : hospitals_) {
        if (hospital.name.find(key) != Text::npos) {
            result.hospitals.push_back(hospital);
        }
    }
    return result;
}

SearchArray<Hospital> SearchEngine::search(const Text& input) const {
    return searchDetailed(input).hospitals;
}
