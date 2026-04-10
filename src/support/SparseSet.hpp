#pragma once

#include <type_traits>

#include "SizedArray.hpp"

/// An extremely fast set data structure (sparse set) at the cost of memory usage.
template <typename T, size_t Capacity, typename IdType>
    requires std::is_trivially_copyable_v<T> && std::is_integral_v<IdType>
class SparseSet {
    struct Element {
        T value;
        IdType id;
    };

    SizedArray<Element, Capacity> elements_;
    std::array<uint32_t, Capacity> idToIndexTable_;

    static constexpr uint32_t NULL_INDEX = std::numeric_limits<uint32_t>::max();

   public:
    SparseSet() { idToIndexTable_.fill(NULL_INDEX); }

    [[nodiscard]] size_t size() const { return elements_.size(); }
    [[nodiscard]] bool empty() const { return elements_.empty(); }

    [[nodiscard]] bool contains(IdType id) const {
        assert(id < Capacity);
        return idToIndexTable_[id] != NULL_INDEX;
    }

    [[nodiscard]] const T& get(IdType id) const {
        assert(contains(id));
        return elements_[idToIndexTable_[id]].value;
    }
    [[nodiscard]] T& get(IdType id) {
        assert(contains(id));
        return elements_[idToIndexTable_[id]].value;
    }

    void insert(T value, IdType id) {
        if (contains(id)) return;

        assert(id < Capacity);
        idToIndexTable_[id] = size();
        elements_.emplace_back(value, id);
    }

    void erase(IdType id) {
        if (!contains(id)) return;

        const uint32_t idx = idToIndexTable_[id];

        Element& lastElem = elements_.back();
        elements_[idx] = std::move(lastElem);

        idToIndexTable_[lastElem.id] = idx;
        // MUST be done after, in case we are erasing the last element
        idToIndexTable_[id] = NULL_INDEX;

        elements_.pop_back();
    }

    [[nodiscard]] const T& operator[](size_t index) const { return elements_[index].value; }
    [[nodiscard]] T& operator[](size_t index) { return elements_[index].value; }
};
