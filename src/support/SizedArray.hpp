#pragma once

#include <algorithm>
#include <stdexcept>

/// Imitates a std::vector without memory allocation, by using a fixed-size capacity.
template <typename T, size_t Capacity>
class SizedArray {
    T data_[Capacity];
    size_t size_ = 0;

   public:
    SizedArray() = default;

    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] static size_t capacity() { return Capacity; }
    [[nodiscard]] bool empty() const { return size_ == 0; }

    void push_back(const T& value) {
        if (size_ >= Capacity) [[unlikely]] {
            throw std::out_of_range("SizedArray capacity exceeded");
        }
        data_[size_++] = value;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        push_back(T(std::forward<Args>(args)...));
    }

    void pop_back() {
        if (size_ == 0) [[unlikely]] {
            throw std::out_of_range("SizedArray is empty");
        }
        --size_;
    }

    [[nodiscard]] const T& back() const {
        if (size_ == 0) [[unlikely]] {
            throw std::out_of_range("SizedArray is empty");
        }
        return data_[size_ - 1];
    }

    [[nodiscard]] T& back() {
        if (size_ == 0) [[unlikely]] {
            throw std::out_of_range("SizedArray is empty");
        }
        return data_[size_ - 1];
    }

    void clear() { size_ = 0; }

   public:
    [[nodiscard]] const T* begin() const { return data_; }
    [[nodiscard]] T* begin() { return data_; }
    [[nodiscard]] const T* end() const { return data_ + size_; }
    [[nodiscard]] T* end() { return data_ + size_; }

    [[nodiscard]] const T& operator[](size_t index) const { return data_[index]; }
    [[nodiscard]] T& operator[](size_t index) { return data_[index]; }
};
