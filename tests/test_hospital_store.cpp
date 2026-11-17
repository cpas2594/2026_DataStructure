#include "data/HospitalStore.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void requireTrue(bool ok, const char* expression, const char* file, int line) {
    if (!ok) {
        std::ostringstream message;
        message << file << ':' << line << " CHECK failed: " << expression;
        throw std::runtime_error(message.str());
    }
}
}
#define CHECK(expression) requireTrue(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

namespace {
Hospital makeHospital(const std::string& id, const std::string& name,
                      const std::string& district = "") {
    Hospital h{};
    h.id = id;
    h.name = name;
    h.district = district;
    return h;
}
}  // namespace

int main() {
    try {
        HospitalStore store;
        CHECK(store.empty());
        CHECK(store.findById("A1100001") == nullptr);
        CHECK(store.findByName("서울중앙병원").empty());

        // 1. 저장과 원본 순서 유지
        const std::size_t added = store.addAll({
            makeHospital("A1100001", "서울중앙병원", "중구"),
            makeHospital("A1100002", "한강병원", "용산구"),
            makeHospital("A1100007", "서울중앙병원", "종로구"),  // 같은 이름, 다른 HPID
            makeHospital("A1100001", "중복된행", "강남구"),      // 같은 HPID: 제외
            makeHospital("", "HPID없음"),                        // 빈 HPID: 제외
            makeHospital(" A1100010 ", "  강남응급센터 "),       // 공백은 정리해서 저장
        });
        CHECK(added == 4);
        CHECK(store.size() == 4);
        CHECK(store.hospitals()[0].id == "A1100001");
        CHECK(store.hospitals()[1].id == "A1100002");
        CHECK(store.hospitals()[2].id == "A1100007");
        CHECK(store.hospitals()[3].id == "A1100010");

        // 2. HPID 조회 (해시 테이블)
        const Hospital* found = store.findById("A1100002");
        CHECK(found != nullptr && found->name == "한강병원");
        CHECK(store.findById(" A1100010\t") != nullptr);
        CHECK(store.findById("A9999999") == nullptr);

        // 3. 중복 HPID는 처음 것을 유지하고 기록을 남깁니다.
        CHECK(store.findById("A1100001")->name == "서울중앙병원");
        CHECK(store.duplicates().size() == 1);
        CHECK(store.duplicates()[0].id == "A1100001");
        CHECK(store.duplicates()[0].name == "중복된행");
        CHECK(store.duplicates()[0].keptIndex == 0);

        // 4. 병원명 조회: 같은 이름 두 곳이 저장 순서대로 나옵니다.
        HospitalIndexList same = store.findByName("서울중앙병원");
        CHECK(same.size() == 2 && same[0] == 0 && same[1] == 2);
        CHECK(store.findByName("강남응급센터").size() == 1);
        CHECK(store.findByName("중복된행").empty());  // 제외된 행은 인덱스에도 없습니다.

        std::vector<Hospital> list = store.toHospitals({2, 0, 99});
        CHECK(list.size() == 2);  // 범위 밖 99는 건너뜁니다.
        CHECK(list[0].district == "종로구" && list[1].district == "중구");

        // 5. 저장소로 만든 SearchEngine이 같은 인덱스를 사용합니다.
        SearchEngine engine = store.makeSearchEngine();
        CHECK(engine.search("").size() == 4);
        CHECK(engine.search("A1100007").size() == 1);
        CHECK(engine.search("서울중앙병원").size() == 2);
        CHECK(engine.search("강남응급센터").size() == 1);
        CHECK(engine.search("병원").size() == 3);  // 부분 일치
        CHECK(engine.search("중복된행").empty());

        // 6. 인덱스 크기와 원본 크기가 일치합니다.
        CHECK(store.idIndex().size() == store.size());
        CHECK(store.nameIndex().size() == 3);  // 서울중앙병원, 한강병원, 강남응급센터

        // 6-1. HPID 소문자 입력도 같은 병원으로 취급 (정규화)
        CHECK(store.findById("a1100002") != nullptr);
        CHECK(!store.add(makeHospital("a1100002", "소문자중복")));
        CHECK(store.duplicates().size() == 2);

        // 7. 해시 테이블 rehash가 일어날 만큼 많이 넣어도 모두 찾을 수 있습니다.
        HospitalStore big;
        for (int i = 0; i < 500; ++i) {
            big.add(makeHospital("H" + std::to_string(i), "병원" + std::to_string(i % 50)));
        }
        CHECK(big.size() == 500);
        for (int i = 0; i < 500; ++i) {
            const Hospital* h = big.findById("H" + std::to_string(i));
            CHECK(h != nullptr && h->id == "H" + std::to_string(i));
        }
        CHECK(big.findByName("병원7").size() == 10);
    } catch (const std::exception& e) {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return 1;
    }

    std::cout << "[PASS] test_hospital_store: add, duplicate HPID, empty HPID, name index, SearchEngine link, rehash\n";
    return 0;
}
