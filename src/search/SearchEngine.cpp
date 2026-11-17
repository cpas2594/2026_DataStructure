#include "search/SearchEngine.h"

#include <cctype>

SearchEngine::SearchEngine(const std::vector<Hospital>& hospitals,
                           const HospitalIdIndex& byId,
                           const HospitalNameIndex& byName)
    : hospitals_(hospitals), byId_(byId), byName_(byName) {}

std::string SearchEngine::trim(const std::string& input) {
    std::size_t start = 0;
    while (start < input.size() &&
           std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    std::size_t end = input.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string SearchEngine::normalizeName(const std::string& name) {
    return trim(name);
}

std::string SearchEngine::normalizeId(const std::string& id) {
    std::string result = trim(id);
    for (char& c : result) {
        const unsigned char byte = static_cast<unsigned char>(c);
        if (byte < 0x80) {
            c = static_cast<char>(std::toupper(byte));
        }
    }
    return result;
}

bool SearchEngine::addToIndexes(const Hospital& hospital, std::size_t index,
                                HospitalIdIndex& byId, HospitalNameIndex& byName) {
    const std::string id = normalizeId(hospital.id);
    if (id.empty() || !byId.insert(id, index)) {
        return false;
    }
    const std::string name = normalizeName(hospital.name);
    if (name.empty()) {
        return true;
    }
    if (std::vector<std::string>* ids = byName.find(name)) {
        ids->push_back(id);
    } else {
        byName.insert(name, std::vector<std::string>{id});
    }
    return true;
}

SearchResult SearchEngine::searchDetailed(const std::string& input) const {
    SearchResult result;
    const std::string key = normalizeName(input);

    if (key.empty()) {
        result.matchType = MatchType::All;
        result.hospitals = hospitals_;
        return result;
    }

    const std::string idKey = normalizeId(key);
    if (const std::size_t* index = byId_.find(idKey)) {
        if (*index < hospitals_.size() &&
            normalizeId(hospitals_[*index].id) == idKey) {
            result.matchType = MatchType::ExactId;
            result.hospitals.push_back(hospitals_[*index]);
            return result;
        }
    }

    if (const std::vector<std::string>* ids = byName_.find(key)) {
        result.hospitals.reserve(ids->size());
        for (const std::string& id : *ids) {
            const std::size_t* index = byId_.find(id);
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
        if (hospital.name.find(key) != std::string::npos) {
            result.hospitals.push_back(hospital);
        }
    }
    return result;
}

std::vector<Hospital> SearchEngine::search(const std::string& input) const {
    return searchDetailed(input).hospitals;
}
