// 병합 정렬 정확성 및 성능 테스트. (std::stable_sort는 정답 비교용으로만 사용)
// 병원명 -> HPID 정렬, 빈 배열/원소 하나, 안정성, std::stable_sort 결과와 비교, 대용량 시간 측정.

#include "algorithms/HospitalComparator.h"
#include "algorithms/MergeSort.hpp"
#include "model/Hospital.h"
#include "structures/DynamicArray.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <exception>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
void requireTrue(bool ok, const char* expression, int line) {
    if (!ok) {
        std::ostringstream message;
        message << __FILE__ << ':' << line << " CHECK failed: " << expression;
        throw std::runtime_error(message.str());
    }
}

#define CHECK(expression) requireTrue(static_cast<bool>(expression), #expression, __LINE__)

Hospital makeHospital(const std::string& id, const std::string& name) {
    Hospital hospital{};
    hospital.id = id;
    hospital.name = name;
    return hospital;
}

bool lessInt(int a, int b) {
    return a < b;
}

template <typename T>
bool sameValues(const DynamicArray<T>& actual, const DynamicArray<T>& expected) {
    if (actual.size() != expected.size()) {
        return false;
    }
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (!(actual[i] == expected[i])) {
            return false;
        }
    }
    return true;
}

void testEmptyAndSingle() {
    DynamicArray<Hospital> empty;
    mergeSort(empty, hospitalNameThenIdLess);
    CHECK(empty.empty());

    DynamicArray<Hospital> single = {makeHospital("A1100011", "가톨릭대학교여의도성모병원")};
    mergeSort(single, hospitalNameThenIdLess);
    CHECK(single.size() == 1);
    CHECK(single[0].id == "A1100011");
}

void testNameThenId() {
    // 실제 CSV의 병원 이름과 HPID를 사용합니다. 같은 이름은 HPID 순서를 확인하려고 만든 값입니다.
    DynamicArray<Hospital> hospitals = {
        makeHospital("A1121013", "가톨릭대학교은평성모병원"),
        makeHospital("A1100037", "대림성모병원"),
        makeHospital("A1100011", "가톨릭대학교여의도성모병원"),
        makeHospital("Z0000002", "같은이름병원"),
        makeHospital("Z0000001", "같은이름병원"),
    };
    mergeSort(hospitals, hospitalNameThenIdLess);

    CHECK(hospitals.size() == 5);
    CHECK(hospitals[0].name == "가톨릭대학교여의도성모병원");
    CHECK(hospitals[1].name == "가톨릭대학교은평성모병원");
    CHECK(hospitals[2].id == "Z0000001");  // 이름이 같으면 HPID 오름차순
    CHECK(hospitals[3].id == "Z0000002");
    CHECK(hospitals[4].name == "대림성모병원");
}

void testAlreadySortedAndReversed() {
    DynamicArray<int> sorted = {1, 2, 3, 4, 5, 6, 7};
    mergeSort(sorted, lessInt);
    CHECK(sameValues(sorted, DynamicArray<int>{1, 2, 3, 4, 5, 6, 7}));

    DynamicArray<int> reversed = {7, 6, 5, 4, 3, 2, 1};
    mergeSort(reversed, lessInt);
    CHECK(sameValues(reversed, DynamicArray<int>{1, 2, 3, 4, 5, 6, 7}));

    DynamicArray<int> duplicates = {3, 1, 3, 2, 1, 3};
    mergeSort(duplicates, lessInt);
    CHECK(sameValues(duplicates, DynamicArray<int>{1, 1, 2, 3, 3, 3}));
}

void testStability() {
    // 비교 기준(첫 번째 값)이 같으면 입력 순서(두 번째 값)가 유지되어야 합니다.
    DynamicArray<std::pair<int, int>> items = {{2, 0}, {1, 1}, {2, 2}, {1, 3}, {2, 4}, {1, 5}};
    mergeSort(items, [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return a.first < b.first;
    });
    const DynamicArray<std::pair<int, int>> expected = {{1, 1}, {1, 3}, {1, 5}, {2, 0}, {2, 2}, {2, 4}};
    CHECK(sameValues(items, expected));
}

// 무작위 병원 목록을 만듭니다. 이름 후보를 적게 두어 같은 이름이 자주 나오게 합니다.
DynamicArray<Hospital> makeRandomHospitals(std::size_t count, std::mt19937& rng) {
    const DynamicArray<std::string> names = {
        "가톨릭대학교여의도성모병원", "가톨릭대학교은평성모병원", "대림성모병원",
        "서울대학교병원", "강남세브란스병원", "Seoul Clinic", "365병원", "가나병원"};
    std::uniform_int_distribution<std::size_t> pickName(0, names.size() - 1);
    std::uniform_int_distribution<int> pickId(0, 9999999);

    DynamicArray<Hospital> hospitals;
    hospitals.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        std::string id = std::to_string(pickId(rng));
        id = "A" + std::string(7 - id.size(), '0') + id;
        hospitals.push_back(makeHospital(id, names[pickName(rng)]));
    }
    return hospitals;
}

bool sameOrder(const DynamicArray<Hospital>& a, const DynamicArray<Hospital>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].id != b[i].id || a[i].name != b[i].name) {
            return false;
        }
    }
    return true;
}

void testMatchesStdStableSort() {
    // 여러 크기의 무작위 입력에서 표준 라이브러리의 안정 정렬과 결과가 같아야 합니다.
    std::mt19937 rng(2026);
    for (std::size_t count : {2u, 3u, 10u, 77u, 1000u}) {
        for (int round = 0; round < 20; ++round) {
            DynamicArray<Hospital> mine = makeRandomHospitals(count, rng);
            DynamicArray<Hospital> expected = mine;
            mergeSort(mine, hospitalNameThenIdLess);
            std::stable_sort(expected.begin(), expected.end(), hospitalNameThenIdLess);
            CHECK(sameOrder(mine, expected));
        }
    }
}

double elapsedMs(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

void testPerformance() {
    // 실제 데이터는 77건이지만, O(n log n) 동작을 확인하려고 크기를 늘려 측정합니다.
    // 시간은 컴퓨터마다 다르므로 결과만 검증하고, 시간은 참고용으로 출력합니다.
    std::mt19937 rng(11);
    std::cout << "[INFO] performance (mergeSort vs std::stable_sort)\n";
    for (std::size_t count : {77u, 1000u, 10000u, 100000u}) {
        const DynamicArray<Hospital> input = makeRandomHospitals(count, rng);

        DynamicArray<Hospital> mine = input;
        auto start = std::chrono::steady_clock::now();
        mergeSort(mine, hospitalNameThenIdLess);
        const double mineMs = elapsedMs(start);

        DynamicArray<Hospital> expected = input;
        start = std::chrono::steady_clock::now();
        std::stable_sort(expected.begin(), expected.end(), hospitalNameThenIdLess);
        const double stdMs = elapsedMs(start);

        CHECK(sameOrder(mine, expected));
        std::cout << "       n=" << count << "  mergeSort=" << mineMs
                  << "ms  std::stable_sort=" << stdMs << "ms\n";
    }
}
}  // namespace

int main() {
    try {
        testEmptyAndSingle();
        testNameThenId();
        testAlreadySortedAndReversed();
        testStability();
        testMatchesStdStableSort();
        testPerformance();
        std::cout << "[PASS] test_sort: empty/single, name->HPID, sorted/reversed, stability, "
                     "matches std::stable_sort, performance\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
