#pragma once

// 역할: 사용자가 입력한 검색어와 선택 조건을 전달하는 공통 자료형.
// TODO: 검색어, 지역, 병원 종류, 응급의료기관 종류와 조건 미선택 표현 정의.
#pragma once

// 역할: 사용자가 입력한 검색어와 선택 조건을 전달하는 공통 자료형
// 빈 문자열은 "선택 안 함"을 뜻함 (HospitalFilter의 FilterConditions와 같은
// 규칙)

#include <string>

struct SearchQuery {
  std::string keyword;        // 병원명 또는 HPID. 비어 있으면 전체 목록
  std::string district;       // 자치구 (예: 영등포구)
  std::string hospitalType;   // 병원 종류 (예: 종합병원)
  std::string emergencyType;  // 응급의료기관 종류 (예: 지역응급의료센터)
};