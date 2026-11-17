#pragma once

#include <string>
#include <vector>

struct Hospital;

struct FilterConditions {
    std::string district;  // 빈 문자열은 지역 미선택입니다.
    std::string hospitalType;  // 빈 문자열은 병원 종류 미선택입니다.
};

class HospitalFilter {
public:
    // 선택한 자치구와 병원 종류가 완전히 일치하는 병원을 입력 순서대로 반환합니다.
    // 미선택 조건은 검사하지 않으며, 원본 후보는 변경하지 않습니다.
    static std::vector<Hospital> apply(
        const std::vector<Hospital>& candidates,
        const FilterConditions& conditions
    );
};
