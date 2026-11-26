#include "structures/BasicTypes.hpp"
#include "data/HospitalStore.h"



namespace {



volatile size_t g_sink = 0;

Text makeId(size_t i) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "A%07zu", i);
    return buffer;
}

SearchArray<Hospital> makeHospitals(size_t n) {
    SearchArray<Hospital> hospitals;
    hospitals.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        Hospital h{};
        h.id = makeId(i);
        h.name = "서울병원" + numberText(i / 2);
        h.district = "강남구";
        h.hospitalType = "종합병원";
        h.emergencyType = "지역응급의료기관";
        hospitals.push_back(h);
    }
    return hospitals;
}

template <typename Fn>
double averageMicros(size_t repeat, Fn fn) {
    const auto start = secondsNow();
    for (size_t r = 0; r < repeat; ++r) {
        fn(r);
    }
    const double elapsed = (secondsNow() - start) * 1000000.0;
    return elapsed / static_cast<double>(repeat);
}

size_t stringBytes(const Text& s) {
    return sizeof(Text) + (s.capacity() + 1);
}

size_t estimateIndexBytes(const HospitalStore& store) {
    const HospitalIdIndex& byId = store.idIndex();
    const HospitalNameIndex& byName = store.nameIndex();
    size_t bytes = byId.bucketCount() * sizeof(void*);
    bytes += byName.bucketCount() * sizeof(void*);
    for (const Hospital& h : store.hospitals()) {
        bytes += stringBytes(h.id) + sizeof(size_t) + sizeof(void*);
        bytes += stringBytes(h.id);
    }
    bytes += byName.size() *
             (sizeof(Text) + sizeof(void*) + sizeof(SearchArray<Text>));
    return bytes;
}

}

int main() {
    const SearchArray<size_t> sizes = {1000, 5000, 10000, 50000, 100000, 200000};

    printf("%8s | %10s %10s %10s %10s | %6s %8s %7s | %10s\n",
                "N", "HPID해시", "HPID순차", "이름해시", "부분검색",
                "버킷", "최장체인", "적재율", "인덱스KB");
    printf("---------+---------------------------------------------+--------------------------+-----------\n");

    for (size_t n : sizes) {
        HospitalStore store;
        store.addAll(makeHospitals(n));
        const SearchEngine engine = store.makeSearchEngine();
        const SearchArray<Hospital>& all = store.hospitals();

        const size_t fastRepeat = 20000;
        const size_t slowRepeat = n >= 50000 ? 20 : 200;

        const double idHash = averageMicros(fastRepeat, [&](size_t r) {
            g_sink = g_sink + engine.search(makeId((r * 7919) % n)).size();
        });
        const double idLinear = averageMicros(slowRepeat, [&](size_t r) {
            const Text key = makeId((r * 7919) % n);
            for (const Hospital& h : all) {
                if (h.id == key) { g_sink = g_sink + 1; break; }
            }
        });
        const double nameHash = averageMicros(fastRepeat, [&](size_t r) {
            g_sink = g_sink + engine.search("서울병원" + numberText((r * 7919) % (n / 2))).size();
        });
        const double partial = averageMicros(slowRepeat, [&](size_t) {
            g_sink = g_sink + engine.search("병원99").size();
        });

        printf("%8zu | %10.3f %10.3f %10.3f %10.3f | %6zu %8zu %7.2f | %10.1f\n",
                    n, idHash, idLinear, nameHash, partial,
                    store.idIndex().bucketCount(), store.idIndex().longestChain(),
                    store.idIndex().loadFactor(),
                    static_cast<double>(estimateIndexBytes(store)) / 1024.0);
    }

    printf("\n시간 단위: 검색 1회 평균(마이크로초). 해시 검색은 N이 커져도 거의 일정하고,\n"
                "순차/부분 검색은 N에 비례해 늘어나는지 확인합니다.\n");
    return 0;
}
