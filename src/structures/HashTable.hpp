#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// 박수연 담당: 문자열 key 기반 해시 테이블
// 충돌 처리: separate chaining (버킷별 vector<Entry>)
// 평균 탐색 O(1), 최악 O(N) (문자열 해싱/비교 비용 별도)
template <typename Value>
class HashTable {
public:
    explicit HashTable(std::size_t initialBucketCount = 8)
        : buckets_(initialBucketCount == 0 ? 8 : initialBucketCount), count_(0) {}

    bool insert(const std::string& key, const Value& value) {
        if (find(key) != nullptr) {
            return false;
        }

        if ((count_ + 1) * 4 > buckets_.size() * 3) {
            rehash(buckets_.size() * 2);
        }
        buckets_[bucketIndex(key)].push_back({key, value});
        ++count_;
        return true;
    }

    Value* find(const std::string& key) {
        auto& bucket = buckets_[bucketIndex(key)];
        for (auto& entry : bucket) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    const Value* find(const std::string& key) const {
        const auto& bucket = buckets_[bucketIndex(key)];
        for (const auto& entry : bucket) {
            if (entry.first == key) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    std::size_t bucketIndex(const std::string& key) const {
        return std::hash<std::string>{}(key) % buckets_.size();
    }

    std::size_t size() const { return count_; }
    std::size_t bucketCount() const { return buckets_.size(); }
    bool empty() const { return count_ == 0; }

    void clear() {
        for (auto& bucket : buckets_) {
            bucket.clear();
        }
        count_ = 0;
    }

private:
    using Entry = std::pair<std::string, Value>;
    std::vector<std::vector<Entry>> buckets_;
    std::size_t count_;

    void rehash(std::size_t newBucketCount) {
        std::vector<std::vector<Entry>> newBuckets(newBucketCount);
        for (auto& bucket : buckets_) {
            for (auto& entry : bucket) {
                const std::size_t index =
                    std::hash<std::string>{}(entry.first) % newBucketCount;
                newBuckets[index].push_back(std::move(entry));
            }
        }
        buckets_.swap(newBuckets);
    }
};
