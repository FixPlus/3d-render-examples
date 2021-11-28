//
// Created by Бушев Дмитрий on 31.10.2021.
//

#ifndef RENDERAPITEST_UNIVERSALCONTAINER_H
#define RENDERAPITEST_UNIVERSALCONTAINER_H

#include <map>
#include <stack>
namespace APITest {

template <typename T> class KeyProducer {
  T topKey = 0;
  std::stack<T> freeKeys_;

public:
  T nextKey() {
    if (freeKeys_.empty()) {
      return topKey++;
    } else {
      auto ret = freeKeys_.top();
      freeKeys_.pop();
      return ret;
    }
  }
  void freeKey(T key) {
    if (key == (topKey - 1))
      topKey--;
    else
      freeKeys_.push(key);
  }
  void clear() {
    topKey = 0;
    while (!freeKeys_.empty())
      freeKeys_.pop();
  }
};

template <typename T> class UniversalContainer {
  std::map<uint32_t, T> container_;
  KeyProducer<uint32_t> keyProducer_;

public:
  template <typename... Args> uint32_t emplace(Args... args) {
    return container_
        .emplace(std::piecewise_construct, keyProducer_.nextKey(), args...)
        .first->first;
  }

  T &get(uint32_t key) { return container_.at(key); }
  T const &get(uint32_t key) const { return container_.at(key); }

  uint32_t push(T element) {
    return container_.insert({keyProducer_.nextKey(), std::forward<T>(element)})
        .first->first;
  }

  void erase(uint32_t key) {
    container_.erase(key);
    keyProducer_.freeKey(key);
  }

  void clear() {
    container_.clear();
    keyProducer_.clear();
  }

  typename std::map<uint32_t, T>::iterator begin() noexcept {
    return container_.begin();
  }

  typename std::map<uint32_t, T>::iterator end() noexcept {
    return container_.end();
  }

  typename std::map<uint32_t, T>::const_iterator begin() const noexcept {
    return container_.begin();
  }

  typename std::map<uint32_t, T>::const_iterator end() const noexcept {
    return container_.end();
  }
};
} // namespace APITest
#endif // RENDERAPITEST_UNIVERSALCONTAINER_H
