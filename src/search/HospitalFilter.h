#pragma once

#include <string>
#include <vector>

struct Hospital;

struct FilterConditions {
    std::string district;  // 빈 문자열은 지역 미선택입니다.
    std::string hospitalType;  // 빈 문자열은 병원 종류 미선택입니다.
    std::string emergencyType;  // 빈 문자열은 응급의료기관 종류 미선택입니다.
};

class HospitalFilter {
public:
    // 선택한 조건을 모두 만족하는 병원만 반환합니다(AND, 각 조건은 완전 일치).
    // 빈 조건은 통과하며, 모든 조건이 비어 있으면 전체 후보를 반환합니다.
    // 원본 후보는 변경하지 않고, 결과는 입력 순서를 유지합니다.
    static std::vector<Hospital> apply(
        const std::vector<Hospital>& candidates,
        const FilterConditions& conditions
    );
};
