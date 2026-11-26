#include "data/HospitalStore.h"

#include <chrono>
#include <cstdio>
#include <string>
#include "structures/DynamicArray.hpp"

namespace {

using Clock = std::chrono::steady_clock;

volatile std::size_t g_sink = 0;

std::string makeId(std::size_t i) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "A%07zu", i);
    return buffer;
}

DynamicArray<Hospital> makeHospitals(std::size_t n) {
    DynamicArray<Hospital> hospitals;
    hospitals.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        Hospital h{};
        h.id = makeId(i);
        h.name = "서울병원" + std::to_string(i / 2);
        h.district = "강남구";
        h.hospitalType = "종합병원";
        h.emergencyType = "지역응급의료기관";
        hospitals.push_back(h);
    }
    return hospitals;
}

template <typename Fn>
double averageMicros(std::size_t repeat, Fn fn) {
    const auto start = Clock::now();
    for (std::size_t r = 0; r < repeat; ++r) {
        fn(r);
    }
    const auto elapsed = std::chrono::duration<double, std::micro>(Clock::now() - start);
    return elapsed.count() / static_cast<double>(repeat);
}

std::size_t stringBytes(const std::string& s) {
    return sizeof(std::string) + (s.capacity() > 15 ? s.capacity() + 1 : 0);
}

std::size_t estimateIndexBytes(const HospitalStore& store) {
    const HospitalIdIndex& byId = store.idIndex();
    const HospitalNameIndex& byName = store.nameIndex();
    // Approximate actual chaining storage (excludes allocator overhead).
    struct IdNode { std::string key; std::size_t value; void* next; };
    struct NameNode { std::string key; DynamicArray<std::string> value; void* next; };
    std::size_t bytes = (byId.bucketCount() + byName.bucketCount()) * sizeof(void*);
    bytes += byId.size() * sizeof(IdNode) + byName.size() * sizeof(NameNode);
    for (const Hospital& h : store.hospitals()) {
        bytes += stringBytes(h.id) - sizeof(std::string); // ID key storage
        const auto* ids = byName.find(h.name);
        if (ids && !ids->empty() && (*ids)[0] == h.id) {
            bytes += stringBytes(h.name) - sizeof(std::string);
            bytes += ids->capacity() * sizeof(std::string);
            for (const auto& id : *ids) bytes += stringBytes(id) - sizeof(std::string);
        }
    }
    return bytes;
}

}

int main() {
    const DynamicArray<std::size_t> sizes = {1000, 5000, 10000, 50000, 100000, 200000};

    std::printf("%8s | %10s %10s %10s %10s | %6s %8s %7s | %10s\n",
                "N", "HPID해시", "HPID순차", "이름해시", "부분검색",
                "버킷", "최장체인", "적재율", "인덱스KB");
    std::printf("---------+---------------------------------------------+--------------------------+-----------\n");

    for (std::size_t n : sizes) {
        HospitalStore store;
        store.addAll(makeHospitals(n));
        const SearchEngine engine = store.makeSearchEngine();
        const DynamicArray<Hospital>& all = store.hospitals();

        const std::size_t fastRepeat = 20000;
        const std::size_t slowRepeat = n >= 50000 ? 20 : 200;

        const double idHash = averageMicros(fastRepeat, [&](std::size_t r) {
            g_sink = g_sink + engine.search(makeId((r * 7919) % n)).size();
        });
        const double idLinear = averageMicros(slowRepeat, [&](std::size_t r) {
            const std::string key = makeId((r * 7919) % n);
            for (const Hospital& h : all) {
                if (h.id == key) { g_sink = g_sink + 1; break; }
            }
        });
        const double nameHash = averageMicros(fastRepeat, [&](std::size_t r) {
            g_sink = g_sink + engine.search("서울병원" + std::to_string((r * 7919) % (n / 2))).size();
        });
        const double partial = averageMicros(slowRepeat, [&](std::size_t) {
            g_sink = g_sink + engine.search("병원99").size();
        });

        std::printf("%8zu | %10.3f %10.3f %10.3f %10.3f | %6zu %8zu %7.2f | %10.1f\n",
                    n, idHash, idLinear, nameHash, partial,
                    store.idIndex().bucketCount(), store.idIndex().longestChain(),
                    store.idIndex().loadFactor(),
                    static_cast<double>(estimateIndexBytes(store)) / 1024.0);
    }

    std::printf("\n시간 단위: 검색 1회 평균(마이크로초). 해시 검색은 N이 커져도 거의 일정하고,\n"
                "순차/부분 검색은 N에 비례해 늘어나는지 확인합니다.\n");
    return 0;
}
