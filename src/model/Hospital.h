#pragma once

#include <string>

// 역할: 병원 한 곳의 정보를 표현하는 공통 자료형.
// 검색(SearchEngine), 필터(HospitalFilter), 저장소(HospitalStore)가 함께 사용합니다.
// 모든 값은 CSV에서 읽은 문자열을 앞뒤 공백만 제거해 그대로 보관합니다.
struct Hospital {
    std::string id;             // HPID (CSV: 기관ID), 병원 고유 키
    std::string name;           // 병원명 (CSV: 기관명)
    std::string district;       // 자치구 (주소에서 추출, 예: "영등포구")
    std::string hospitalType;   // 병원 종류 (CSV: 병원분류명, 예: "종합병원")
    std::string emergencyType;  // 응급의료기관 종류 (CSV: 응급의료기관코드명)
    std::string address;        // 주소 (CSV: 주소)
    std::string phone;          // 대표 전화 (CSV: 대표전화1)
    std::string emergencyPhone; // 응급실 전화 (CSV: 응급실전화)
    bool emergencyRoomOpen = false; // 응급실 운영 여부 (CSV: 응급실운영여부 == "1")
};
