#include "data/HospitalStore.h"

bool HospitalStore::add(const Hospital& hospital) {
    Hospital normalized = hospital;
    normalized.id = SearchEngine::normalizeId(hospital.id);
    normalized.name = SearchEngine::normalizeName(hospital.name);
    if (normalized.id.empty()) {
        return false;
    }

    if (const std::size_t* kept = byId_.find(normalized.id)) {
        duplicates_.push_back({normalized.id, normalized.name, *kept});
        return false;
    }

    const std::size_t index = hospitals_.size();
    if (!SearchEngine::addToIndexes(normalized, index, byId_, byName_)) {
        return false;
    }

    hospitals_.push_back(normalized);
    return true;
}

std::size_t HospitalStore::addAll(const std::vector<Hospital>& hospitals) {
    std::size_t added = 0;
    for (const Hospital& hospital : hospitals) {
        if (add(hospital)) {
            ++added;
        }
    }
    return added;
}

const Hospital* HospitalStore::findById(const std::string& id) const {
    const std::size_t* index = byId_.find(SearchEngine::normalizeId(id));
    if (index == nullptr || *index >= hospitals_.size()) {
        return nullptr;
    }
    return &hospitals_[*index];
}

HospitalIndexList HospitalStore::findByName(const std::string& name) const {
    HospitalIndexList result;
    const std::vector<std::string>* ids =
        byName_.find(SearchEngine::normalizeName(name));
    if (ids == nullptr) {
        return result;
    }
    result.reserve(ids->size());
    for (const std::string& id : *ids) {
        if (const std::size_t* index = byId_.find(id)) {
            result.push_back(*index);
        }
    }
    return result;
}

std::vector<Hospital> HospitalStore::toHospitals(const HospitalIndexList& indexes) const {
    std::vector<Hospital> result;
    result.reserve(indexes.size());
    for (std::size_t index : indexes) {
        if (index < hospitals_.size()) {
            result.push_back(hospitals_[index]);
        }
    }
    return result;
}

SearchEngine HospitalStore::makeSearchEngine() const {
    return SearchEngine(hospitals_, byId_, byName_);
}
