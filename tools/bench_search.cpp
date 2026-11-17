// 박수연 담당: 검색 성능 측정
//
// 제안서 완료 기준: "데이터 크기 N 증가에 따른 검색 응답 시간과 메모리 사용량을 측정한다."
// 실제 CSV는 77행이라 차이가 잘 드러나지 않으므로, 같은 형식의 병원 데이터를 N개 만들어 측정합니다.
//
// 측정 항목 (한 번 검색당 평균 시간, 마이크로초)
//   HPID 해시 : 해시 테이블 HPID 완전 일치        -> 평균 O(1)
//   HPID 순차 : 같은 검색을 배열 순차 탐색으로    -> O(N)  (해시 테이블과 비교용 기준선)
//   이름 해시 : 병원명 완전 일치 (같은 이름 2곳)  -> 평균 O(1 + B)
//   부분 검색 : 병원명 부분 일치                 -> O(N)
// 메모리: 인덱스가 차지하는 대략적인 바이트 수 (문자열 capacity + 버킷 배열 + 엔트리 크기)
//
// 실행: ./build/bench_search        (Release 빌드 권장: cmake -DCMAKE_BUILD_TYPE=Release)

#include "data/HospitalStore.h"

#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

// 결과를 최적화로 지우지 못하게 합니다.
volatile std::size_t g_sink = 0;

std::string makeId(std::size_t i) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "A%07zu", i);
    return buffer;
}

std::vector<Hospital> makeHospitals(std::size_t n) {
    std::vector<Hospital> hospitals;
    hospitals.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        Hospital h{};
        h.id = makeId(i);
        // 2곳씩 같은 이름(중복 병원명)을 갖도록 합니다.
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

// 인덱스(byId, byName)의 대략적인 메모리 사용량
std::size_t estimateIndexBytes(const HospitalStore& store) {
    const HospitalIdIndex& byId = store.idIndex();
    const HospitalNameIndex& byName = store.nameIndex();
    // 버킷 배열: 버킷마다 vector 객체 하나
    std::size_t bytes = byId.bucketCount() * sizeof(std::vector<std::pair<std::string, std::size_t>>);
    bytes += byName.bucketCount() * sizeof(std::vector<std::pair<std::string, std::vector<std::string>>>);
    for (const Hospital& h : store.hospitals()) {
        bytes += stringBytes(h.id) + sizeof(std::size_t);  // byId 엔트리
        bytes += stringBytes(h.id);                        // byName의 HPID 목록 원소
    }
    // byName 엔트리: 이름 key + vector 객체 (이름 수 = N / 2)
    bytes += byName.size() *
             (sizeof(std::string) + 16 + sizeof(std::vector<std::string>));
    return bytes;
}

}  // namespace

int main() {
    const std::vector<std::size_t> sizes = {1000, 5000, 10000, 50000, 100000, 200000};

    std::printf("%8s | %10s %10s %10s %10s | %6s %8s %7s | %10s\n",
                "N", "HPID해시", "HPID순차", "이름해시", "부분검색",
                "버킷", "최장체인", "적재율", "인덱스KB");
    std::printf("---------+---------------------------------------------+--------------------------+-----------\n");

    for (std::size_t n : sizes) {
        HospitalStore store;
        store.addAll(makeHospitals(n));
        const SearchEngine engine = store.makeSearchEngine();
        const std::vector<Hospital>& all = store.hospitals();

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
