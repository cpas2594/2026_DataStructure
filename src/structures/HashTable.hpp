#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// 박수연 담당: 문자열 key 기반 해시 테이블
template <typename Value>
class HashTable {
public:
    static constexpr std::size_t kDefaultBucketCount = 8;

    explicit HashTable(std::size_t initialBucketCount = kDefaultBucketCount)
        : buckets_(initialBucketCount == 0 ? kDefaultBucketCount : initialBucketCount),
          count_(0) {}

    static std::uint64_t hashString(const std::string& key) {
        std::uint64_t hash = 14695981039346656037ULL;
        for (unsigned char byte : key) {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    std::size_t bucketIndex(const std::string& key) const {
        return static_cast<std::size_t>(hashString(key) % buckets_.size());
    }

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

    bool contains(const std::string& key) const { return find(key) != nullptr; }

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

    double loadFactor() const {
        return static_cast<double>(count_) / static_cast<double>(buckets_.size());
    }

    std::size_t longestChain() const {
        std::size_t longest = 0;
        for (const auto& bucket : buckets_) {
            if (bucket.size() > longest) {
                longest = bucket.size();
            }
        }
        return longest;
    }

    std::size_t collisionCount() const { return count_ - usedBucketCount(); }

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
