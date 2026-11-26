#include "structures/BasicTypes.hpp"
#include "data/HospitalStore.h"



namespace {
void requireTrue(bool ok, const char* expression, const char* file, int line) {
    if (!ok) {
        throw ProjectError(Text(file) + ":" + numberText(line) + " CHECK failed: " + expression);
    }
}
}
#define CHECK(expression) requireTrue(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

namespace {
Hospital makeHospital(const Text& id, const Text& name,
                      const Text& district = "") {
    Hospital h{};
    h.id = id;
    h.name = name;
    h.district = district;
    return h;
}
}

int main() {
    try {
        HospitalStore store;
        CHECK(store.empty());
        CHECK(store.findById("A1100001") == nullptr);
        CHECK(store.findByName("서울중앙병원").empty());

        const size_t added = store.addAll({
            makeHospital("A1100001", "서울중앙병원", "중구"),
            makeHospital("A1100002", "한강병원", "용산구"),
            makeHospital("A1100007", "서울중앙병원", "종로구"),
            makeHospital("A1100001", "중복된행", "강남구"),
            makeHospital("", "HPID없음"),
            makeHospital(" A1100010 ", "  강남응급센터 "),
        });
        CHECK(added == 4);
        CHECK(store.size() == 4);
        CHECK(store.hospitals()[0].id == "A1100001");
        CHECK(store.hospitals()[1].id == "A1100002");
        CHECK(store.hospitals()[2].id == "A1100007");
        CHECK(store.hospitals()[3].id == "A1100010");

        const Hospital* found = store.findById("A1100002");
        CHECK(found != nullptr && found->name == "한강병원");
        CHECK(store.findById(" A1100010\t") != nullptr);
        CHECK(store.findById("A9999999") == nullptr);

        CHECK(store.findById("A1100001")->name == "서울중앙병원");
        CHECK(store.duplicates().size() == 1);
        CHECK(store.duplicates()[0].id == "A1100001");
        CHECK(store.duplicates()[0].name == "중복된행");
        CHECK(store.duplicates()[0].keptIndex == 0);

        HospitalIndexList same = store.findByName("서울중앙병원");
        CHECK(same.size() == 2 && same[0] == 0 && same[1] == 2);
        CHECK(store.findByName("강남응급센터").size() == 1);
        CHECK(store.findByName("중복된행").empty());

        SearchArray<Hospital> list = store.toHospitals({2, 0, 99});
        CHECK(list.size() == 2);
        CHECK(list[0].district == "종로구" && list[1].district == "중구");

        SearchEngine engine = store.makeSearchEngine();
        CHECK(engine.search("").size() == 4);
        CHECK(engine.search("A1100007").size() == 1);
        CHECK(engine.search("서울중앙병원").size() == 2);
        CHECK(engine.search("강남응급센터").size() == 1);
        CHECK(engine.search("병원").size() == 3);
        CHECK(engine.search("중복된행").empty());

        CHECK(store.idIndex().size() == store.size());
        CHECK(store.nameIndex().size() == 3);

        CHECK(store.findById("a1100002") != nullptr);
        CHECK(!store.add(makeHospital("a1100002", "소문자중복")));
        CHECK(store.duplicates().size() == 2);

        HospitalStore big;
        for (int i = 0; i < 500; ++i) {
            big.add(makeHospital("H" + numberText(i), "병원" + numberText(i % 50)));
        }
        CHECK(big.size() == 500);
        for (int i = 0; i < 500; ++i) {
            const Hospital* h = big.findById("H" + numberText(i));
            CHECK(h != nullptr && h->id == "H" + numberText(i));
        }
        CHECK(big.findByName("병원7").size() == 10);
    } catch (const ProjectError& e) {
        consoleErr << "[FAIL] " << e.what() << '\n';
        return 1;
    }

    consoleOut << "[PASS] test_hospital_store: add, duplicate HPID, empty HPID, name index, SearchEngine link, rehash\n";
    return 0;
}
