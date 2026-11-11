#pragma once

#include <string>
#include <vector>

struct Hospital;

struct FilterConditions {
    std::string district;  // 빈 문자열은 지역 미선택입니다.
};

class HospitalFilter {
public:
    // 자치구가 완전히 일치하는 병원을 입력 순서대로 복사해 반환합니다.
    // 지역 미선택 시 전체 후보를 반환하며, 원본 후보는 변경하지 않습니다.
    static std::vector<Hospital> apply(
        const std::vector<Hospital>& candidates,
        const FilterConditions& conditions
    );
};
