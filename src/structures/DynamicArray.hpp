#pragma once

#include <cstddef>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

// 복사 가능한 T를 연속된 공간에 저장합니다. 원소에 기본 생성자는 필요하지 않습니다.
// 용량이 늘어나면 기존 원소의 주소와 반복자는 무효가 됩니다.
template <typename T>
class DynamicArray {
public:
    DynamicArray() = default;

    DynamicArray(std::initializer_list<T> values) : DynamicArray() {
        reserve(values.size());
        for (const T& value : values) {
            appendCopy(value);
        }
    }

    DynamicArray(const DynamicArray& other) : DynamicArray() {
        reserve(other.size_);
        for (const T& value : other) {
            appendCopy(value);
        }
    }

    DynamicArray(DynamicArray&& other) noexcept { swapStorage(other); }

    DynamicArray& operator=(DynamicArray other) noexcept {
        swapStorage(other);
        return *this;
    }

    ~DynamicArray() {
        while (size_ > 0) {
            data_[--size_].~T();
        }
        if (data_) {
            std::allocator<T>{}.deallocate(data_, capacity_);
        }
    }

    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    T& operator[](std::size_t index) noexcept { return data_[index]; }
    const T& operator[](std::size_t index) const noexcept { return data_[index]; }

    T* begin() noexcept { return data_; }
    const T* begin() const noexcept { return data_; }
    T* end() noexcept { return size_ ? data_ + size_ : data_; }
    const T* end() const noexcept { return size_ ? data_ + size_ : data_; }

    void reserve(std::size_t requested) {
        if (requested > capacity_) {
            DynamicArray replacement = copyWithCapacity(requested);
            swapStorage(replacement);
        }
    }

    void push_back(const T& value) {
        if (size_ < capacity_) {
            appendCopy(value);
            return;
        }
        if (size_ == maxCapacity()) {
            throw std::length_error("DynamicArray capacity exceeded");
        }
        const std::size_t next = capacity_ == 0 ? 1 :
            (capacity_ > maxCapacity() / 2 ? maxCapacity() : capacity_ * 2);
        DynamicArray replacement = copyWithCapacity(next);
        // value가 현재 배열의 원소인 경우에도 복사를 마친 뒤 저장 공간을 교체합니다.
        replacement.appendCopy(value);
        swapStorage(replacement);
    }

private:
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;

    static constexpr std::size_t maxCapacity() noexcept {
        return (std::numeric_limits<std::ptrdiff_t>::max)() / sizeof(T);
    }

    void appendCopy(const T& value) {
        ::new (static_cast<void*>(data_ + size_)) T(value);
        ++size_;
    }

    DynamicArray copyWithCapacity(std::size_t requested) const {
        if (requested > maxCapacity()) {
            throw std::length_error("DynamicArray capacity exceeded");
        }
        DynamicArray result;
        // allocator는 정렬된 메모리만 확보합니다. 실제 원소의 생성·소멸은 배열이 담당합니다.
        result.data_ = std::allocator<T>{}.allocate(requested);
        result.capacity_ = requested;
        for (const T& value : *this) {
            result.appendCopy(value);
        }
        return result;
    }

    void swapStorage(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }
};
