#include "search/SearchService.h"

// TODO: SearchEngine -> HospitalFilter -> MergeSort 순서로 처리 흐름 연결.
// 각 알고리즘의 세부 구현은 해당 모듈에 둡니다.
#include "algorithms/HospitalComparator.h"
#include "algorithms/MergeSort.hpp"
#include "search/HospitalFilter.h"
#include "search/SearchService.h"

SearchService::SearchService(const HospitalStore& store)
    : engine_(store.makeSearchEngine()) {}

SearchOutcome SearchService::search(const SearchQuery& query) const {
  SearchOutcome outcome;

  // 1. 검색: SearchEngine은 std::vector로 돌려주므로 DynamicArray로 옮겨 담음
  const SearchResult found = engine_.searchDetailed(query.keyword);
  outcome.matchType = found.matchType;
  outcome.candidateCount = found.hospitals.size();
  DynamicArray<Hospital> candidates;
  candidates.reserve(found.hospitals.size());
  for (const Hospital& hospital : found.hospitals) {
    candidates.push_back(hospital);
  }

  // 2. 필터: 선택한 조건만 AND로 적용 (빈 조건은 통과)
  FilterConditions conditions;
  conditions.district = query.district;
  conditions.hospitalType = query.hospitalType;
  conditions.emergencyType = query.emergencyType;
  outcome.hospitals = HospitalFilter::apply(candidates, conditions);

  // 3. 정렬: 병원명 -> HPID 순, 안정 병합 정렬
  mergeSort(outcome.hospitals, hospitalNameThenIdLess);
  return outcome;
}