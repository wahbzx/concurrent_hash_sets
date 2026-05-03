#ifndef HASH_SET_STRIPED_H
#define HASH_SET_STRIPED_H

#include "src/hash_set_base.h"
#include <atomic>
#include <cassert>
#include <mutex>
#include <vector>

template <typename T> class HashSetStriped : public HashSetBase<T> {
private:
  std::vector<std::vector<T>> table_; // Hash Table - Vector of Vectors
  std::mutex *locks_;                 // Array of Locks
  size_t lock_count_;            // Count to keep track of the number of locks
  std::atomic<size_t> setSize_;  // Size of the hash set
  std::atomic<size_t> capacity_; // Capacity of the set - Number of buckets

  // Hash Function
  size_t HashObj(const T &elem) const {
    return std::hash<T>{}(elem) % capacity_.load();
  }

  bool Policy() { return setSize_.load() / capacity_.load() > 4; }

  void ResizeTable() {
    // Keep track of the capacity in case another thread is also attempting to
    // resize
    size_t old_capacity = capacity_.load();

    // Lock all the locks in ascending order
    for (size_t i = 0; i < lock_count_; i++) {
      locks_[i].lock();
    }

    // If someone has resized while were acquiring all the locks then abort
    // resizing the table
    if (capacity_ != old_capacity) {
      // unlock all locks
      for (size_t i = 0; i < lock_count_; i++) {
        locks_[i].unlock();
      }
      return;
    }

    // Begin Resizing the table
    capacity_.store(capacity_.load() * 2);
    std::vector<std::vector<T>> new_table(capacity_.load(), std::vector<T>());
    for (auto bucket : table_) {
      for (T item : bucket) {
        new_table[HashObj(item)].push_back(item);
      }
    }
    table_ = new_table;

    // Unlock all the locks
    for (size_t i = 0; i < lock_count_; i++) {
      locks_[i].unlock();
    }
  }

public:
  explicit HashSetStriped(size_t capacity) {
    table_.resize(capacity);
    setSize_.store(0);
    capacity_.store(capacity);
    // Allocate a list of mutexes
    locks_ = new std::mutex[capacity];
    lock_count_ = capacity;
  }
  // Destructor to delete the list of locks to avoid memory leak
  ~HashSetStriped() override { delete[] locks_; }

  bool Add(T elem) final {
    // Resize at the beginning to avoid aborting due to another thread
    // attempting to Add in the middle of resizing
    if (Policy()) {
      ResizeTable();
    }

    // Acquire the relevant lock
    std::scoped_lock<std::mutex> lock(
        locks_[std::hash<T>()(elem) % lock_count_]);

    // Find whether the object is already present
    size_t bucket = HashObj(elem);
    for (const auto &item : table_[bucket]) {
      // If element found then return false
      if (item == elem) {
        return false;
      }
    }
    // Add the element to the bucket and return true
    table_[bucket].push_back(elem);
    setSize_.fetch_add(1);

    return true;
  }

  bool Remove(T elem) final {
    // Acquire the relevant lock
    std::scoped_lock<std::mutex> lock(
        locks_[std::hash<T>{}(elem) % lock_count_]);
    size_t bucket = HashObj(elem);
    // Find element in the bucket
    for (auto it = table_[bucket].begin(); it != table_[bucket].end(); ++it) {
      // If element is found then remove it from the bucket and return true
      if (*it == elem) {
        table_[bucket].erase(it);
        setSize_.fetch_sub(1);
        return true;
      }
    }
    // Otherwise the element was not found and we return false
    return false;
  }

  [[nodiscard]] bool Contains(T elem) final {
    // Acquire the relevant lock
    std::scoped_lock<std::mutex> lock(
        locks_[std::hash<T>{}(elem) % lock_count_]);

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

  [[nodiscard]] size_t Size() const final { return setSize_.load(); }
};

#endif // HASH_SET_STRIPED_H
