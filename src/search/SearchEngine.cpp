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
        if (byte < 0x80) {  // ASCII만 바꿉니다. UTF-8 멀티바이트는 그대로 둡니다.
            c = static_cast<char>(std::toupper(byte));
        }
    }
    return result;
}

bool SearchEngine::addToIndexes(const Hospital& hospital, std::size_t index,
                                HospitalIdIndex& byId, HospitalNameIndex& byName) {
    const std::string id = normalizeId(hospital.id);
    if (id.empty() || !byId.insert(id, index)) {
        return false;  // 빈 HPID 또는 중복 HPID
    }
    const std::string name = normalizeName(hospital.name);
    if (name.empty()) {
        return true;  // 이름이 없으면 HPID로만 찾을 수 있습니다.
    }
    if (std::vector<std::string>* ids = byName.find(name)) {
        ids->push_back(id);  // 중복 병원명: 같은 key의 HPID 목록에 추가
    } else {
        byName.insert(name, std::vector<std::string>{id});
    }
    return true;
}

SearchResult SearchEngine::searchDetailed(const std::string& input) const {
    SearchResult result;
    const std::string key = normalizeName(input);

    // 1. 빈 검색어: 전체 목록
    if (key.empty()) {
        result.matchType = MatchType::All;
        result.hospitals = hospitals_;
        return result;
    }

    // 2. HPID 완전 일치: 평균 O(1)
    const std::string idKey = normalizeId(key);
    if (const std::size_t* index = byId_.find(idKey)) {
        if (*index < hospitals_.size() &&
            normalizeId(hospitals_[*index].id) == idKey) {
            result.matchType = MatchType::ExactId;
            result.hospitals.push_back(hospitals_[*index]);
            return result;
        }
    }

    // 3. 병원명 완전 일치: 평균 O(1 + B), B = 같은 이름의 병원 수
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

    // 4. 병원명 부분 일치: 배열 순차 탐색 O(N)
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
