#include "structures/DynamicArray.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
#define CHECK(expr) do { if (!(expr)) throw std::logic_error(#expr); } while (false)

void testGrowth() {
    DynamicArray<int> numbers;
    CHECK(numbers.empty() && numbers.begin() == numbers.end());
    for (int i = 0; i < 64; ++i) {
        numbers.push_back(i);
    }
    CHECK(numbers.size() == 64 && numbers.capacity() == 64);
    numbers.push_back(numbers[3]);  // 확장 중에 자기 배열의 원소를 추가
    CHECK(numbers.capacity() == 128 && numbers[64] == 3);
    numbers.reserve(200);
    numbers.reserve(10);
    CHECK(numbers.capacity() == 200 && numbers.size() == 65);
    int i = 0;
    for (int value : numbers) {
        CHECK(value == (i == 64 ? 3 : i));
        ++i;
    }
    CHECK(i == 65);
}

void testCopyAndMove() {
    DynamicArray<std::string> original = {"서울", "부산"};
    auto copy = original;
    copy[0] = "인천";
    CHECK(original[0] == "서울");
    DynamicArray<std::string> assigned = {"대구"};
    assigned = copy;
    copy[0] = "수원";
    CHECK(assigned[0] == "인천" && assigned[1] == "부산");
    assigned = assigned;
    auto moved = std::move(assigned);
    CHECK(assigned.empty() && moved.size() == 2);
    original = std::move(moved);
    CHECK(moved.empty() && original[0] == "인천");
    moved.push_back("서울");
    CHECK(moved[0] == "서울");
}

// 두 번째 원소를 복사할 때 실패시켜 부분 생성된 원소까지 해제하는지 확인합니다.
struct Tracked {
    static inline int alive = 0;
    static inline bool fail = false;
    int id;

    explicit Tracked(int idValue) : id(idValue) { ++alive; }
    Tracked(const Tracked& other) : id(other.id) {
        if (fail && id == 2) throw std::runtime_error("copy failed");
        ++alive;
    }
    ~Tracked() { --alive; }
};

void testLifetime() {
    {
        DynamicArray<Tracked> items = {Tracked(1), Tracked(2)};
        CHECK(Tracked::alive == 2);
        Tracked::fail = true;
        bool failed = false;
        try {
            items.reserve(10);
        } catch (const std::runtime_error&) {
            failed = true;
        }
        Tracked::fail = false;
        CHECK(failed && Tracked::alive == 2);
        CHECK(items.size() == 2 && items[0].id == 1 && items[1].id == 2);
        items.reserve(10);
        CHECK(Tracked::alive == 2);
    }
    CHECK(Tracked::alive == 0);
}
}

int main() {
    try {
        testGrowth();
        testCopyAndMove();
        testLifetime();
        std::cout << "[PASS] DynamicArray growth, copy, move and lifetime\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
