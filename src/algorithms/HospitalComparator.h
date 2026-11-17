#pragma once

// 역할: 검색 결과 정렬 기준(병원명 -> HPID) 정의
// MergeSort의 비교 함수로 전달해서 사용
//   mergeSort(results, hospitalNameThenIdLess);

#include "model/Hospital.h"

inline bool hospitalNameThenIdLess(const Hospital& a, const Hospital& b) {
    if (a.name != b.name) {
        return a.name < b.name;
    }
    return a.id < b.id;
}