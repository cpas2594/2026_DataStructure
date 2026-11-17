#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// 박수연 담당: 문자열 key 기반 해시 테이블
//
// - 해시 함수: FNV-1a (64비트). std::hash 대신 직접 구현해 플랫폼마다 같은 버킷 번호를 냅니다.
//   UTF-8 한글 병원명도 바이트 단위로 섞으므로 그대로 key로 쓸 수 있습니다.
// - 충돌 처리: separate chaining (버킷마다 vector<Entry> 체인)
// - 적재율(load factor) = size / bucketCount 가 0.75를 넘으면 버킷을 2배로 늘려 rehash
// - 평균 탐색 O(1), 최악 O(N) (모든 key가 한 버킷에 모일 때. 문자열 해싱/비교 비용 별도)
//
// 같은 병원명에 여러 HPID가 연결되는 경우는 Value를 vector<string>(HPID 목록)으로 두어 처리합니다.
// (HospitalNameIndex = HashTable<std::vector<std::string>>)
template <typename Value>
class HashTable {
public:
    static constexpr std::size_t kDefaultBucketCount = 8;

    explicit HashTable(std::size_t initialBucketCount = kDefaultBucketCount)
        : buckets_(initialBucketCount == 0 ? kDefaultBucketCount : initialBucketCount),
          count_(0) {}

    // FNV-1a 64비트 해시. 같은 문자열은 항상 같은 값을 냅니다.
    static std::uint64_t hashString(const std::string& key) {
        std::uint64_t hash = 14695981039346656037ULL;  // FNV offset basis
        for (unsigned char byte : key) {
            hash ^= byte;                 // ① 바이트를 섞고
            hash *= 1099511628211ULL;     // ② FNV prime을 곱한다
        }
        return hash;
    }

    // key가 들어갈 버킷 번호: 0 … bucketCount() - 1
    std::size_t bucketIndex(const std::string& key) const {
        return static_cast<std::size_t>(hashString(key) % buckets_.size());
    }

    // 새 key를 저장합니다. 이미 있는 key면 false를 반환하고 아무것도 바꾸지 않습니다.
    bool insert(const std::string& key, const Value& value) {
        if (find(key) != nullptr) {
            return false;
        }
        // 넣은 뒤 적재율이 0.75를 넘을 것 같으면 먼저 버킷을 늘립니다.
        if ((count_ + 1) * 4 > buckets_.size() * 3) {
            rehash(buckets_.size() * 2);
        }
        buckets_[bucketIndex(key)].push_back({key, value});
        ++count_;
        return true;
    }

    // key의 값 주소를 반환합니다. 없으면 nullptr. 해당 key의 체인 하나만 확인합니다.
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

    bool contains(const std::string& key) const { return find(key) != nullptr; }

    // key를 지웁니다. 없으면 false. 체인 안의 순서는 바뀔 수 있습니다(마지막 원소와 교환 후 제거).
    bool remove(const std::string& key) {
        auto& bucket = buckets_[bucketIndex(key)];
        for (std::size_t i = 0; i < bucket.size(); ++i) {
            if (bucket[i].first == key) {
                if (i + 1 != bucket.size()) {
                    bucket[i] = std::move(bucket.back());
                }
                bucket.pop_back();
                --count_;
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return count_; }
    std::size_t bucketCount() const { return buckets_.size(); }
    bool empty() const { return count_ == 0; }

    // 적재율: 버킷 하나에 평균 몇 개의 key가 있는가
    double loadFactor() const {
        return static_cast<double>(count_) / static_cast<double>(buckets_.size());
    }

    // 가장 긴 체인의 길이 = 이 테이블에서 find 한 번의 최악 비교 횟수
    std::size_t longestChain() const {
        std::size_t longest = 0;
        for (const auto& bucket : buckets_) {
            if (bucket.size() > longest) {
                longest = bucket.size();
            }
        }
        return longest;
    }

    // 충돌 횟수: 각 버킷에서 첫 key 다음에 들어온 key의 수 (= size - 사용 중인 버킷 수)
    std::size_t collisionCount() const { return count_ - usedBucketCount(); }

    // key가 하나 이상 들어 있는 버킷 수
    std::size_t usedBucketCount() const {
        std::size_t used = 0;
        for (const auto& bucket : buckets_) {
            if (!bucket.empty()) {
                ++used;
            }
        }
        return used;
    }

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

    // 모든 key를 새 버킷 수로 다시 배치합니다. 버킷 번호가 bucketCount에 따라 달라지기 때문입니다.
    void rehash(std::size_t newBucketCount) {
        std::vector<std::vector<Entry>> newBuckets(newBucketCount);
        for (auto& bucket : buckets_) {
            for (auto& entry : bucket) {
                const std::size_t index =
                    static_cast<std::size_t>(hashString(entry.first) % newBucketCount);
                newBuckets[index].push_back(std::move(entry));
            }
        }
        buckets_.swap(newBuckets);
    }
};
