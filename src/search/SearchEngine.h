#pragma once

#include "model/Hospital.h"
#include "structures/HashTable.hpp"
#include <cstddef>
#include <string>
#include <vector>

// 박수연 담당: 병원명·HPID 검색
//
//   byId   : HPID(정규화)   -> hospitals 배열 인덱스
//   byName : 병원명(정규화) -> 그 이름을 가진 HPID 목록 (중복 병원명 처리)
using HospitalIdIndex = HashTable<std::size_t>;
using HospitalNameIndex = HashTable<std::vector<std::string>>;

// 검색이 어떤 경로로 결과를 찾았는지 나타냅니다. (UI 안내 문구, 테스트, 성능 측정에 사용)
enum class MatchType {
    All,           // 빈 검색어: 전체 목록
    ExactId,       // HPID 완전 일치 (해시 테이블, 평균 O(1))
    ExactName,     // 병원명 완전 일치 (해시 테이블, 평균 O(1 + B))
    PartialName,   // 병원명 부분 일치 (배열 순차 탐색, O(N))
};

struct SearchResult {
    MatchType matchType = MatchType::All;
    std::vector<Hospital> hospitals;  // 원본 배열 순서를 유지합니다.
};

class SearchEngine {
public:
    SearchEngine(const std::vector<Hospital>& hospitals,
                 const HospitalIdIndex& byId,
                 const HospitalNameIndex& byName);

    // 검색 우선순위:
    //   1) 빈 검색어         -> 전체 병원
    //   2) HPID 완전 일치    -> 병원 1곳 (HPID는 대소문자 구분 없음)
    //   3) 병원명 완전 일치  -> 같은 이름의 병원 전부(중복 병원명)
    //   4) 그 밖             -> 병원명에 검색어가 포함된 병원 전부
    // HPID와 병원명이 동시에 일치하는 경우 HPID가 우선합니다(HPID는 고유 키이므로).
    SearchResult searchDetailed(const std::string& input) const;

    // 결과 목록만 필요할 때 사용합니다.
    std::vector<Hospital> search(const std::string& input) const;

    // 앞뒤 공백(스페이스, 탭, 줄바꿈 등) 제거
    static std::string trim(const std::string& input);

    // 병원명 정규화: 앞뒤 공백 제거. 인덱스 키와 검색어 모두 이 함수를 거칩니다.
    static std::string normalizeName(const std::string& name);

    // HPID 정규화: 앞뒤 공백 제거 + 영문 대문자화 ("a1100011" -> "A1100011")
    static std::string normalizeId(const std::string& id);

    // 저장소/테스트에서 인덱스를 같은 규칙으로 만들 수 있도록 제공합니다.
    // index 위치의 병원을 두 인덱스에 등록합니다. HPID가 비었거나 이미 있으면 false.
    static bool addToIndexes(const Hospital& hospital, std::size_t index,
                             HospitalIdIndex& byId, HospitalNameIndex& byName);

private:
    const std::vector<Hospital>& hospitals_;
    const HospitalIdIndex& byId_;
    const HospitalNameIndex& byName_;
};
