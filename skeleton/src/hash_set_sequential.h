#ifndef HASH_SET_SEQUENTIAL_H
#define HASH_SET_SEQUENTIAL_H

#include "src/hash_set_base.h"
#include <cassert>
#include <vector>

template <typename T> class HashSetSequential : public HashSetBase<T> {
private:
  // Hash Table is a vector of vectors (Closed-Addressing)
  std::vector<std::vector<T>> table_;
  size_t setSize_ = 0;
  size_t capacity_;

  // Hash Function
  size_t HashObj(const T &elem) const {
    return std::hash<T>{}(elem) % capacity_;
  }

  bool Policy() { return setSize_ / capacity_ > 4; }

  void ResizeTable() {
    capacity_ *= 2;
    std::vector<std::vector<T>> new_table(capacity_, std::vector<T>());
    for (auto bucket : table_) {
      for (T item : bucket) {
        new_table[HashObj(item)].push_back(item);
      }
    }
    table_ = new_table;
  }

public:
  explicit HashSetSequential(size_t capacity) : capacity_(capacity) {
    table_.resize(capacity);
  }

  bool Add(T elem) final {
    // Hash elem to get the address of the bucket
    size_t bucket = HashObj(elem);
    for (const auto &item : table_[bucket]) {
      // If element found then return false
      if (item == elem) {
        return false;
      }
    }
    // Element not found so add and check if resize is required
    table_[bucket].push_back(elem);
    ++setSize_;
    if (Policy()) {
      ResizeTable();
    }
    return true;
  }

  bool Remove(T elem) final {
    size_t bucket = HashObj(elem);
    for (auto it = table_[bucket].begin(); it != table_[bucket].end(); ++it) {
      // Element found and then removed
      if (*it == elem) {
        table_[bucket].erase(it);
        --setSize_;
        return true;
      }
    }
    // Element not found
    return false;
  }

  [[nodiscard]] bool Contains(T elem) final {
    size_t bucket = HashObj(elem);
    for (const auto &item : table_[bucket]) {
      // Element found in the set
      if (item == elem) {
        return true;
      }
    }
    // Element not found in the set
    return false;
  }

  [[nodiscard]] size_t Size() const final { return setSize_; }
};

#endif // HASH_SET_SEQUENTIAL_H
