#include "structures/HashTable.hpp"

#include <stdexcept>
#include <sstream>
#include <cstddef>
#include <iostream>
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

int main() {
    HashTable<int> table;
    CHECK(table.empty());
    CHECK(table.find("없는병원") == nullptr);
  
    CHECK(table.insert("A1100001", 1));
    CHECK(table.insert("A1100002", 2));
    CHECK(!table.insert("A1100001", 999));
    CHECK(table.size() == 2);
    CHECK(*table.find("A1100001") == 1);

    HashTable<int> collisions(16);
    const std::size_t target = collisions.bucketIndex("collision-base");
    std::string other;
    for (int i = 0; i < 2000; ++i) {
        const std::string key = "different-key-" + std::to_string(i);
        if (key != "collision-base" && collisions.bucketIndex(key) == target) {
            other = key;
            break;
        }
    }
    CHECK(!other.empty());
    CHECK(collisions.insert("collision-base", 10));
    CHECK(collisions.insert(other, 20));
    CHECK(*collisions.find("collision-base") == 10);
    CHECK(*collisions.find(other) == 20);
    CHECK(collisions.find("third-key-missing") == nullptr);

    HashTable<int> growing(2);
    for (int i = 0; i < 300; ++i) {
        CHECK(growing.insert("K" + std::to_string(i), i));
    }
    CHECK(growing.bucketCount() > 2);
    CHECK(growing.size() == 300);
    for (int i = 0; i < 300; ++i) {
        CHECK(*growing.find("K" + std::to_string(i)) == i);
    }
    const HashTable<int>& readonly = growing;
    CHECK(*readonly.find("K299") == 299);

    HashTable<std::vector<std::string>> names;
    CHECK(names.insert("서울중앙병원", {"A1100001"}));
    names.find("서울중앙병원")->push_back("A1100007");
    CHECK(names.size() == 1);
    CHECK(names.find("서울중앙병원")->size() == 2);

    growing.clear();
    CHECK(growing.empty());
    CHECK(growing.find("K299") == nullptr);

    // 직접 구현한 FNV-1a 해시: 같은 key는 항상 같은 값, 알려진 기준값과 일치
    CHECK(HashTable<int>::hashString("") == 14695981039346656037ULL);
    CHECK(HashTable<int>::hashString("a") == 0xaf63dc4c8601ec8cULL);
    CHECK(HashTable<int>::hashString("서울중앙병원") == HashTable<int>::hashString("서울중앙병원"));
    CHECK(HashTable<int>::hashString("A1100001") != HashTable<int>::hashString("A1100002"));

    // remove: 체인 중간/처음 key 삭제, 없는 key 삭제
    CHECK(collisions.remove("collision-base"));
    CHECK(collisions.find("collision-base") == nullptr);
    CHECK(*collisions.find(other) == 20);  // 같은 체인의 다른 key는 그대로
    CHECK(!collisions.remove("collision-base"));
    CHECK(collisions.size() == 1);
    CHECK(collisions.insert("collision-base", 30));  // 지운 key는 다시 넣을 수 있다
    CHECK(*collisions.find("collision-base") == 30);

    // 충돌 통계: 같은 버킷에 두 key -> 가장 긴 체인 2, 충돌 1
    CHECK(collisions.longestChain() == 2);
    CHECK(collisions.collisionCount() == 1);
    CHECK(collisions.usedBucketCount() == 1);
    CHECK(collisions.contains(other));
    CHECK(!collisions.contains("third-key-missing"));

    // 적재율은 rehash 덕분에 항상 0.75 이하
    HashTable<int> load(4);
    for (int i = 0; i < 1000; ++i) {
        CHECK(load.insert("L" + std::to_string(i), i));
        CHECK(load.loadFactor() <= 0.75);
    }
    CHECK(load.size() == 1000);
    CHECK(load.collisionCount() == load.size() - load.usedBucketCount());
    CHECK(load.longestChain() >= 1 && load.longestChain() < 20);

    // 크기 0으로 만들어도 기본 버킷 수를 사용 (0으로 나누지 않는다)
    HashTable<int> zero(0);
    CHECK(zero.bucketCount() == HashTable<int>::kDefaultBucketCount);
    CHECK(zero.insert("x", 1) && *zero.find("x") == 1);

    std::cout << "[PASS] test_hash_table: insert, duplicate, chaining, rehash, name IDs, clear, FNV-1a, remove, collision stats\n";
    return 0;
}
