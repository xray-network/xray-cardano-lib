#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "cardano/core/error.hpp"

namespace cardano::core {

template <typename Key, typename Value, typename Equal = std::equal_to<Key>>
class OrderedMap {
 public:
  using Entry = std::pair<Key, Value>;

  [[nodiscard]] const std::vector<Entry>& entries() const noexcept { return entries_; }
  [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
  [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

  [[nodiscard]] const Value* find(const Key& key) const noexcept {
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
      return equal_(entry.first, key);
    });
    return iterator == entries_.end() ? nullptr : &iterator->second;
  }

  void insert_or_assign(Key key, Value value) {
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
      return equal_(entry.first, key);
    });
    if (iterator == entries_.end()) {
      entries_.emplace_back(std::move(key), std::move(value));
    } else {
      iterator->second = std::move(value);
    }
  }

  [[nodiscard]] bool erase(const Key& key) {
    const auto iterator = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
      return equal_(entry.first, key);
    });
    if (iterator == entries_.end()) {
      return false;
    }
    entries_.erase(iterator);
    return true;
  }

 private:
  std::vector<Entry> entries_;
  [[no_unique_address]] Equal equal_;
};

template <typename Key, typename Value>
using PairMap = std::vector<std::pair<Key, Value>>;

template <typename Value>
class NonEmptyVector {
 public:
  [[nodiscard]] static Result<NonEmptyVector> from(std::vector<Value> values) {
    if (values.empty()) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_structure, "nonempty vector cannot be empty"));
    }
    return NonEmptyVector(std::move(values));
  }

  [[nodiscard]] const std::vector<Value>& values() const noexcept { return values_; }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

 private:
  explicit NonEmptyVector(std::vector<Value> values) : values_(std::move(values)) {}
  std::vector<Value> values_;
};

template <typename Key, typename Value, typename Equal = std::equal_to<Key>>
class NonEmptyMap {
 public:
  [[nodiscard]] static Result<NonEmptyMap> from(OrderedMap<Key, Value, Equal> values) {
    if (values.empty()) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_structure, "nonempty map cannot be empty"));
    }
    return NonEmptyMap(std::move(values));
  }

  [[nodiscard]] const OrderedMap<Key, Value, Equal>& values() const noexcept { return values_; }

 private:
  explicit NonEmptyMap(OrderedMap<Key, Value, Equal> values) : values_(std::move(values)) {}
  OrderedMap<Key, Value, Equal> values_;
};

}  // namespace cardano::core
