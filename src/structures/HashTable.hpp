#pragma once
#include "structures/BasicTypes.hpp"


template <typename Value>
class HashTable {
public:
    static constexpr size_t kDefaultBucketCount = 8;

    explicit HashTable(size_t initialBucketCount = kDefaultBucketCount)
        : bucketCount_(initialBucketCount == 0 ? kDefaultBucketCount
                                               : initialBucketCount),
          buckets_(new Node*[bucketCount_]()), count_(0) {}

    HashTable(const HashTable& other)
        : bucketCount_(other.bucketCount_), buckets_(new Node*[bucketCount_]()),
          count_(0) {
        for (size_t i = 0; i < other.bucketCount_; ++i) {
            for (const Node* node = other.buckets_[i]; node != nullptr;
                 node = node->next) {
                insert(node->key, node->value);
            }
        }
    }

    HashTable& operator=(const HashTable& other) {
        if (this == &other) {
            return *this;
        }
        HashTable copy(other);
        swap(copy);
        return *this;
    }

    ~HashTable() {
        clear();
        delete[] buckets_;
    }

    static uint64_t hashString(const Text& key) {
        uint64_t hash = 14695981039346656037ULL;
        for (unsigned char byte : key) {
            hash ^= byte;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    size_t bucketIndex(const Text& key) const {
        return static_cast<size_t>(hashString(key) % bucketCount_);
    }

    bool insert(const Text& key, const Value& value) {
        if (find(key) != nullptr) {
            return false;
        }
        if ((count_ + 1) * 4 > bucketCount_ * 3) {
            rehash(bucketCount_ * 2);
        }
        const size_t index = bucketIndex(key);
        buckets_[index] = new Node(key, value, buckets_[index]);
        ++count_;
        return true;
    }

    Value* find(const Text& key) {
        for (Node* node = buckets_[bucketIndex(key)]; node != nullptr;
             node = node->next) {
            if (node->key == key) {
                return &node->value;
            }
        }
        return nullptr;
    }

    const Value* find(const Text& key) const {
        for (const Node* node = buckets_[bucketIndex(key)]; node != nullptr;
             node = node->next) {
            if (node->key == key) {
                return &node->value;
            }
        }
        return nullptr;
    }

    bool contains(const Text& key) const { return find(key) != nullptr; }

    bool remove(const Text& key) {
        Node** link = &buckets_[bucketIndex(key)];
        while (*link != nullptr) {
            if ((*link)->key == key) {
                Node* removed = *link;
                *link = removed->next;
                delete removed;
                --count_;
                return true;
            }
            link = &((*link)->next);
        }
        return false;
    }

    size_t size() const { return count_; }
    size_t bucketCount() const { return bucketCount_; }
    bool empty() const { return count_ == 0; }

    double loadFactor() const {
        return static_cast<double>(count_) / static_cast<double>(bucketCount_);
    }

    size_t longestChain() const {
        size_t longest = 0;
        for (size_t i = 0; i < bucketCount_; ++i) {
            size_t chainLength = 0;
            for (const Node* node = buckets_[i]; node != nullptr;
                 node = node->next) {
                ++chainLength;
            }
            if (chainLength > longest) {
                longest = chainLength;
            }
        }
        return longest;
    }

    size_t collisionCount() const { return count_ - usedBucketCount(); }

    size_t usedBucketCount() const {
        size_t used = 0;
        for (size_t i = 0; i < bucketCount_; ++i) {
            if (buckets_[i] != nullptr) {
                ++used;
            }
        }
        return used;
    }

    void clear() {
        for (size_t i = 0; i < bucketCount_; ++i) {
            Node* node = buckets_[i];
            while (node != nullptr) {
                Node* next = node->next;
                delete node;
                node = next;
            }
            buckets_[i] = nullptr;
        }
        count_ = 0;
    }

private:
    struct Node {
        Text key;
        Value value;
        Node* next;

        Node(const Text& nodeKey, const Value& nodeValue, Node* nextNode)
            : key(nodeKey), value(nodeValue), next(nextNode) {}
    };

    size_t bucketCount_;
    Node** buckets_;
    size_t count_;

    void swap(HashTable& other) {
        const size_t oldBucketCount = bucketCount_;
        bucketCount_ = other.bucketCount_;
        other.bucketCount_ = oldBucketCount;

        Node** oldBuckets = buckets_;
        buckets_ = other.buckets_;
        other.buckets_ = oldBuckets;

        const size_t oldCount = count_;
        count_ = other.count_;
        other.count_ = oldCount;
    }

    void rehash(size_t newBucketCount) {
        Node** newBuckets = new Node*[newBucketCount]();
        for (size_t i = 0; i < bucketCount_; ++i) {
            Node* node = buckets_[i];
            while (node != nullptr) {
                Node* next = node->next;
                const size_t index = static_cast<size_t>(
                    hashString(node->key) % newBucketCount);
                node->next = newBuckets[index];
                newBuckets[index] = node;
                node = next;
            }
        }
        delete[] buckets_;
        buckets_ = newBuckets;
        bucketCount_ = newBucketCount;
    }
};
