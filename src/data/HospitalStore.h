#pragma once

#include "model/Hospital.h"
#include "search/SearchEngine.h"  // HospitalIdIndex, HospitalNameIndex

#include <cstddef>
#include <string>
#include <vector>

// 역할: 병원 원본 배열과 검색 인덱스(해시 테이블)를 소유하는 저장소.
// 인덱스 구축 규칙(정규화, 중복 HPID, 중복 병원명)은 SearchEngine::addToIndexes와 같습니다.
//
//   hospitals_ : 원본 배열. 추가된 순서를 그대로 유지합니다.
//   byId_      : HPID -> hospitals_ 배열 인덱스      (HashTable<size_t>)
//   byName_    : 병원명 -> 그 이름을 가진 HPID 목록  (HashTable<vector<string>>)
//
// 같은 HPID가 두 번 들어오면 처음 것만 저장하고, 나중 것은 duplicates()에 기록합니다.
// 같은 병원명은 여러 병원이 가질 수 있으므로 byName_ 값은 HPID 목록입니다.

// 검색 결과로 주고받는 배열 인덱스 목록의 공통 타입입니다.
using HospitalIndexList = std::vector<std::size_t>;

// 저장하지 않고 제외한 행의 기록입니다.
struct DuplicateRecord {
    std::string id;      // 중복된 HPID
    std::string name;    // 제외된 행의 병원명
    std::size_t keptIndex;  // 이미 저장돼 있던 병원의 배열 인덱스
};

class HospitalStore {
public:
    HospitalStore() = default;

    // 병원 하나를 저장하고 두 인덱스에 등록합니다.
    // HPID가 비어 있거나 이미 저장된 HPID면 false를 반환하고 아무것도 바꾸지 않습니다.
    // (중복 HPID는 duplicates()에 기록합니다.)
    bool add(const Hospital& hospital);

    // 여러 병원을 순서대로 add 합니다. 실제로 저장된 개수를 반환합니다.
    std::size_t addAll(const std::vector<Hospital>& hospitals);

    // HPID 완전 일치 조회. 없으면 nullptr. 평균 O(1).
    const Hospital* findById(const std::string& id) const;

    // 병원명 완전 일치 조회. 저장 순서대로 배열 인덱스를 반환합니다. 평균 O(1) + 결과 수.
    HospitalIndexList findByName(const std::string& name) const;

    // 배열 인덱스 목록을 병원 목록으로 바꿉니다. 범위를 벗어난 인덱스는 건너뜁니다.
    std::vector<Hospital> toHospitals(const HospitalIndexList& indexes) const;

    std::size_t size() const { return hospitals_.size(); }
    bool empty() const { return hospitals_.empty(); }

    const std::vector<Hospital>& hospitals() const { return hospitals_; }
    const HospitalIdIndex& idIndex() const { return byId_; }
    const HospitalNameIndex& nameIndex() const { return byName_; }
    const std::vector<DuplicateRecord>& duplicates() const { return duplicates_; }

    // 저장된 데이터를 그대로 쓰는 SearchEngine을 만듭니다.
    // 반환된 엔진은 이 저장소를 참조하므로 저장소보다 오래 쓰면 안 됩니다.
    SearchEngine makeSearchEngine() const;

private:
    std::vector<Hospital> hospitals_;
    HospitalIdIndex byId_;
    HospitalNameIndex byName_;
    std::vector<DuplicateRecord> duplicates_;
};
