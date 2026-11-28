#pragma once

// 역할: UI에서 호출할 전체 검색 기능의 선언.
// TODO: 검색 조건을 받아 최종 결과 목록을 반환하는 인터페이스 선언.
#pragma once

// 역할: UI에서 호출할 전체 검색 기능
// 처리 순서 (발표자료 5쪽): SearchEngine 검색 -> HospitalFilter 필터 ->
// mergeSort 정렬 주의: store는 SearchService보다 오래 살아 있어야 함
// (SearchEngine이 store를 참조만 함)

#include <cstddef>

#include "data/HospitalStore.h"
#include "model/Hospital.h"
#include "model/SearchQuery.h"
#include "search/SearchEngine.h"
#include "structures/DynamicArray.hpp"

struct SearchOutcome {
  MatchType matchType = MatchType::All;  // 검색어가 어떻게 일치했는지
  std::size_t candidateCount = 0;        // 필터 적용 전 후보 수
  DynamicArray<Hospital> hospitals;      // 필터·정렬까지 끝난 최종 결과
};

class SearchService {
 public:
  explicit SearchService(const HospitalStore& store);

  SearchOutcome search(const SearchQuery& query) const;

 private:
  SearchEngine engine_;
};