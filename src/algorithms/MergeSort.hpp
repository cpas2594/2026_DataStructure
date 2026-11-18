#pragma once

// 역할: 검색 결과를 정렬하는 병합 정렬 알고리즘.
// TODO: 정렬 대상과 비교 함수를 받는 병합 정렬 구현.
// 병원명 -> HPID 비교 기준은 호출하는 쪽에서 전달합니다.

#include <cstddef>
#include <utility>
#include <vector>

namespace merge_sort_detail {

// [left, mid)와 [mid, right)는 각각 정렬된 상태
// 두 구간을 buffer에 합친 뒤 items로 되돌려 놓음
template <typename T, typename Compare>
void merge(std::vector<T>& items, std::vector<T>& buffer,
           std::size_t left, std::size_t mid, std::size_t right,
           Compare& less) {
    std::size_t i = left;
    std::size_t j = mid;
    std::size_t k = left;

    while (i < mid && j < right) {
        // 오른쪽이 엄격히 작을 때만 오른쪽을 먼저 넣어야 안정 정렬
        if (less(items[j], items[i])) {
            buffer[k++] = std::move(items[j++]);
        } else {
            buffer[k++] = std::move(items[i++]);
        }
    }
    while (i < mid) {
        buffer[k++] = std::move(items[i++]);
    }
    while (j < right) {
        buffer[k++] = std::move(items[j++]);
    }

    for (std::size_t p = left; p < right; ++p) {
        items[p] = std::move(buffer[p]);
    }
}

// 반열린 구간 [left, right) 정렬
template <typename T, typename Compare>
void sortRange(std::vector<T>& items, std::vector<T>& buffer,
               std::size_t left, std::size_t right, Compare& less) {
    if (right - left < 2) {
        return;
    }
    const std::size_t mid = left + (right - left) / 2;
    sortRange(items, buffer, left, mid, less);
    sortRange(items, buffer, mid, right, less);

    // 이미 순서가 맞으면 병합 생략 (정렬된 입력에서 O(n)에 가깝게 동작)
    if (!less(items[mid], items[mid - 1])) {
        return;
    }
    merge(items, buffer, left, mid, right, less);
}

}  // namespace merge_sort_detail

// items를 less 기준 오름차순으로 제자리 정렬
// less(a, b)는 a가 b보다 앞에 와야 할 때만 true 반환
template <typename T, typename Compare>
void mergeSort(std::vector<T>& items, Compare less) {
    if (items.size() < 2) {
        return;
    }
    std::vector<T> buffer(items);  // 기본 생성자가 없는 타입도 쓸 수 있도록 복사로 크기 맞춤
    merge_sort_detail::sortRange(items, buffer, 0, items.size(), less);
}