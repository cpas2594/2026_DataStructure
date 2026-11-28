#include "structures/HashTable.hpp"

#include <stdexcept>
#include <sstream>
#include <cstddef>
#include <iostream>
#include <string>
#include "structures/DynamicArray.hpp"

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

    HashTable<DynamicArray<std::string>> names;
    CHECK(names.insert("서울중앙병원", {"A1100001"}));
    names.find("서울중앙병원")->push_back("A1100007");
    CHECK(names.size() == 1);
    CHECK(names.find("서울중앙병원")->size() == 2);

    growing.clear();
    CHECK(growing.empty());
    CHECK(growing.find("K299") == nullptr);

    CHECK(HashTable<int>::hashString("") == 14695981039346656037ULL);
    CHECK(HashTable<int>::hashString("a") == 0xaf63dc4c8601ec8cULL);
    CHECK(HashTable<int>::hashString("서울중앙병원") == HashTable<int>::hashString("서울중앙병원"));
    CHECK(HashTable<int>::hashString("A1100001") != HashTable<int>::hashString("A1100002"));

    CHECK(collisions.remove("collision-base"));
    CHECK(collisions.find("collision-base") == nullptr);
    CHECK(*collisions.find(other) == 20);
    CHECK(!collisions.remove("collision-base"));
    CHECK(collisions.size() == 1);
    CHECK(collisions.insert("collision-base", 30));
    CHECK(*collisions.find("collision-base") == 30);

    CHECK(collisions.longestChain() == 2);
    CHECK(collisions.collisionCount() == 1);
    CHECK(collisions.usedBucketCount() == 1);
    CHECK(collisions.contains(other));
    CHECK(!collisions.contains("third-key-missing"));

    HashTable<int> load(4);
    for (int i = 0; i < 1000; ++i) {
        CHECK(load.insert("L" + std::to_string(i), i));
        CHECK(load.loadFactor() <= 0.75);
    }
    CHECK(load.size() == 1000);
    CHECK(load.collisionCount() == load.size() - load.usedBucketCount());
    CHECK(load.longestChain() >= 1 && load.longestChain() < 20);

    HashTable<int> zero(0);
    CHECK(zero.bucketCount() == HashTable<int>::kDefaultBucketCount);
    CHECK(zero.insert("x", 1) && *zero.find("x") == 1);

    std::cout << "[PASS] test_hash_table: insert, duplicate, chaining, rehash, name IDs, clear, FNV-1a, remove, collision stats\n";
    return 0;
}
