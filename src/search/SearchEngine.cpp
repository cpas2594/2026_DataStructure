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

std::vector<Hospital> SearchEngine::search(const std::string& input) const {
    const std::string key = trim(input);

    if (key.empty()) {
        return hospitals_;
    }

    // 1. HPID가 정확히 일치하면 단일 병원 반환 (평균 O(1)).
    if (const std::size_t* index = byId_.find(key)) {
        if (*index < hospitals_.size() && hospitals_[*index].id == key) {
            return {hospitals_[*index]};
        }
    }

    if (const std::vector<std::string>* ids = byName_.find(key)) {
        std::vector<Hospital> result;
        result.reserve(ids->size());
        for (const std::string& id : *ids) {
            const std::size_t* index = byId_.find(id);
            if (index != nullptr && *index < hospitals_.size() &&
                hospitals_[*index].id == id && hospitals_[*index].name == key) {
                result.push_back(hospitals_[*index]);
            }
        }
        return result;
    }
    std::vector<Hospital> result;
    for (const Hospital& hospital : hospitals_) {
        if (hospital.name.find(key) != std::string::npos) {
            result.push_back(hospital);
        }
    }
    return result;
}
