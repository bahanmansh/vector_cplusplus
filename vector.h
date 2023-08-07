#define VECTOR_MEMORY_IMPLEMENTED

#include <iostream>
#include <numeric>
#include <exception>
#include <type_traits>
#include <memory>
#include <algorithm>
#pragma once

template <class A>
class Vector {
 public:
  using ValueType = A;
  using Pointer = ValueType*;
  using ConstPointer = const ValueType*;
  using Reference = ValueType&;
  using ConstReference = const ValueType&;
  using SizeType = size_t;
  using Iterator = A*;
  using ConstIterator = const A*;
  using ReverseIterator = std::reverse_iterator<Iterator>;
  using ConstReverseIterator = std::reverse_iterator<ConstIterator>;
  template <typename B>
  friend bool operator<=(const Vector<B>& start, const Vector<B>& finish);
  template <typename B>
  friend bool operator<(const Vector<B>& start, const Vector<B>& finish);
  template <typename B>
  friend bool operator>(const Vector<B>& start, const Vector<B>& finish);
  template <typename B>
  friend bool operator==(const Vector<B>& start, const Vector<B>& finish);
  template <typename B>
  friend bool operator>=(const Vector<B>& start, const Vector<B>& finish);

  void* Extend() {
    if (cap_ == 0) {
      auto new_a = operator new(sizeof(ValueType));
      return new_a;
    }
    auto new_a = operator new(cap_ * 2 * sizeof(ValueType));
    std::uninitialized_move_n(static_cast<Pointer>(buf_), size_, static_cast<Pointer>(new_a));
    return new_a;
  }
  Vector() : buf_(nullptr), size_(0), cap_(0) {
  }
  explicit Vector(size_t s) : Vector() {
    if (s == 0) {
      return;
    }
    auto new_a = operator new(sizeof(ValueType) * s);
    try {
      std::uninitialized_default_construct_n(static_cast<Pointer>(new_a), s);
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    buf_ = new_a;
    size_ = s;
    cap_ = s;
  }

  Vector(size_t s, ConstReference data) : Vector() {
    if (s == 0) {
      return;
    }
    auto new_a = operator new(sizeof(ValueType) * s);
    try {
      std::uninitialized_fill_n(static_cast<Pointer>(new_a), s, data);
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    cap_ = size_ = s;
    buf_ = new_a;
  }

  template <class Iterator, class = std::enable_if_t<std::is_base_of_v<
                                std::forward_iterator_tag, typename std::iterator_traits<Iterator>::iterator_category>>>
  Vector(Iterator start, Iterator finish) : Vector() {
    if (start == finish) {
      return;
    }
    size_t s = std::distance(start, finish);
    auto new_a = operator new(s * sizeof(A));
    try {
      std::uninitialized_copy(start, finish, static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    size_ = s;
    cap_ = s;
    buf_ = new_a;
  }

  Vector(const std::initializer_list<A>& l) : Vector() {
    if (l.size() == 0) {
      return;
    }
    size_t s = l.size();
    auto new_a = operator new(s * sizeof(ValueType));
    try {
      std::uninitialized_copy(l.begin(), l.end(), static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    size_ = s;
    cap_ = s;
    buf_ = new_a;
  }

  Vector(const Vector<A>& elem) : Vector() {
    if (elem.size_ == 0) {
      return;
    }
    auto new_a = operator new(elem.size_ * sizeof(ValueType));
    try {
      std::uninitialized_copy(elem.begin(), elem.end(), static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    buf_ = new_a;
    size_ = elem.size_;
    cap_ = elem.size_;
  }

  Vector(Vector<A>&& elem) : buf_(elem.buf_), size_(elem.size_), cap_(elem.size_) {
    elem.size_ = 0;
    elem.cap_ = 0;
    elem.buf_ = nullptr;
  }

  Vector& operator=(const Vector<A>& elem) {
    if (this == &elem) {
      return *this;
    }
    if (cap_ < elem.size_) {
      auto new_a = operator new(elem.size_ * sizeof(ValueType));
      try {
        std::uninitialized_copy(elem.begin(), elem.end(), static_cast<Pointer>(new_a));
      } catch (...) {
        operator delete(new_a);
        throw;
      }
      std::destroy(begin(), end());
      operator delete(buf_);
      buf_ = new_a;
      size_ = elem.size_;
      cap_ = elem.size_;
    } else if (size_ > elem.size_) {
      std::copy(elem.begin(), elem.end(), static_cast<Pointer>(buf_));
      std::destroy(begin() + elem.size_, begin() + size_);
      size_ = elem.size_;
    } else {
      std::copy(elem.begin(), elem.begin() + size_, static_cast<Pointer>(buf_));
      std::uninitialized_copy(elem.begin() + size_, elem.end(), static_cast<Pointer>(buf_) + size_);
      size_ = elem.size_;
    }
    return *this;
  }

  Vector& operator=(Vector<A>&& elem) noexcept {
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = elem.buf_;
    size_ = elem.size_;
    cap_ = elem.cap_;
    elem.buf_ = nullptr;
    elem.size_ = 0;
    elem.cap_ = 0;
    return *this;
  }

  Vector& operator=(const std::initializer_list<A>& l) {
    if (cap_ < l.size()) {
      auto new_a = operator new(l.size());
      try {
        std::uninitialized_move(l.begin(), l.end(), static_cast<Pointer>(new_a));
      } catch (...) {
        operator delete(new_a);
        throw;
      }
      buf_ = new_a;
      size_ = cap_ = l.size();
    } else if (size_ > l.size()) {
      std::copy(l.begin(), l.end(), static_cast<Pointer>(buf_));
      std::destroy(begin() + l.size(), begin() + size_);
    } else {
      std::move(l.begin(), l.begin() + size_, static_cast<Pointer>(buf_));
      std::uninitialized_move(l.begin() + size_, l.end(), static_cast<Pointer>(buf_) + size_);
    }
    return *this;
  }

  ~Vector() noexcept {
    if (buf_ != nullptr) {
      std::destroy(begin(), end());
      operator delete(buf_);
      size_ = 0;
      cap_ = 0;
    }
  }

  size_t Size() const noexcept {
    return size_;
  }
  bool Empty() const noexcept {
    return size_ == 0;
  }
  size_t Capacity() const noexcept {
    return cap_;
  }

  Reference operator[](size_t id) noexcept {
    return *(static_cast<Pointer>(buf_) + id);
  }

  ConstReference operator[](size_t id) const noexcept {
    return *(static_cast<Pointer>(buf_) + id);
  }
  void Out(size_t id, size_t s) const {
    if (id >= s) {
      throw std::out_of_range{"Vector out of range"};
    }
  }
  Reference At(size_t id) {
    Out(id, size_);
    return static_cast<Pointer>(buf_)[id];
  }

  ConstReference At(size_t id) const {
    Out(id, size_);
    return static_cast<Pointer>(buf_)[id];
  }
  ConstReference Front() const noexcept {
    return static_cast<Pointer>(buf_)[0];
  }
  Reference Front() noexcept {
    return static_cast<Pointer>(buf_)[0];
  }
  ConstReference Back() const noexcept {
    return static_cast<Pointer>(buf_)[size_ - 1];
  }
  Reference Back() noexcept {
    return static_cast<Pointer>(buf_)[size_ - 1];
  }
  ConstPointer Data() const noexcept {
    return static_cast<Pointer>(buf_);
  }
  Pointer Data() noexcept {
    return static_cast<Pointer>(buf_);
  }

  void Swap(Vector<A>& elem) noexcept {
    std::swap(cap_, elem.cap_);
    std::swap(buf_, elem.buf_);
    std::swap(size_, elem.size_);
  }
  void Resizetry(void* a, size_t s, size_t new_s, void* new_a) {
    try {
      std::uninitialized_move(static_cast<Pointer>(a), static_cast<Pointer>(a) + s, static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
    }
    try {
      std::uninitialized_default_construct(static_cast<Pointer>(new_a) + s, static_cast<Pointer>(new_a) + new_s);
    } catch (...) {
      std::move(static_cast<Pointer>(new_a), static_cast<Pointer>(new_a) + s, static_cast<Pointer>(a));
      operator delete(new_a);
      throw;
    }
  }
  void Resize(size_t new_s) {
    if (new_s <= cap_) {
      if (new_s < size_) {
        std::destroy(static_cast<Pointer>(buf_) + new_s, static_cast<Pointer>(buf_) + size_);
      } else {
        std::uninitialized_default_construct(static_cast<Pointer>(buf_) + size_, static_cast<Pointer>(buf_) + new_s);
      }
      size_ = new_s;
      return;
    }

    auto new_a = operator new(new_s * sizeof(ValueType));
    Resizetry(buf_, size_, new_s, new_a);
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    size_ = new_s;
    cap_ = new_s;
  }

  void Resize(size_t new_s, const A& data) {
    if (new_s <= cap_) {
      if (new_s < size_) {
        std::destroy(static_cast<Pointer>(buf_) + new_s, static_cast<Pointer>(buf_) + size_);
      } else {
        std::uninitialized_fill(static_cast<Pointer>(buf_) + size_, static_cast<Pointer>(buf_) + new_s, data);
      }
      size_ = new_s;
      return;
    }
    auto new_a = operator new(new_s * sizeof(ValueType));
    try {
      std::uninitialized_move(static_cast<Pointer>(buf_), static_cast<Pointer>(buf_) + size_, static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
    }
    try {
      std::uninitialized_fill(static_cast<Pointer>(new_a) + size_, static_cast<Pointer>(new_a) + new_s, data);
    } catch (...) {
      std::move(static_cast<Pointer>(new_a), static_cast<Pointer>(new_a) + size_, static_cast<Pointer>(buf_));
      operator delete(new_a);
      throw;
    }
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    size_ = new_s;
    cap_ = new_s;
  }
  void Reservetry(void* new_a) {
    try {
      std::uninitialized_move(begin(), end(), static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
      throw;
    }
  }
  void Reserve(size_t new_cap) {
    if (new_cap <= cap_) {
      return;
    }
    auto new_a = operator new(sizeof(ValueType) * new_cap);
    Reservetry(new_a);
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    cap_ = new_cap;
  }

  void ShrinkToFit() {
    if (size_ == cap_) {
      return;
    }

    if (size_ == 0) {
      operator delete(buf_);
      buf_ = nullptr;
      size_ = 0;
      cap_ = 0;
      return;
    }

    auto new_a = operator new(sizeof(A) * size_);
    try {
      std::uninitialized_move(begin(), end(), static_cast<Pointer>(new_a));
    } catch (...) {
      operator delete(new_a);
      throw;
    }
    std::destroy(begin(), end());
    operator delete(buf_);
    cap_ = size_;
    buf_ = new_a;
  }

  void Clear() noexcept {
    if (size_ > 0) {
      std::destroy(begin(), end());
      size_ = 0;
    }
  }
  void PushBack(const A& data) {
    if (size_ < cap_) {
      new (static_cast<Pointer>(buf_) + size_) A(data);
      size_++;
      return;
    }

    void* new_a = nullptr;
    try {
      new_a = Extend();
      new (static_cast<Pointer>(new_a) + size_) A(data);
    } catch (...) {
      std::destroy_n(static_cast<Pointer>(new_a), size_);
      operator delete(new_a);
      throw;
    }
    cap_ = (cap_ == 0) ? 1 : cap_ * 2;
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    size_++;
  }

  void PushBack(A&& data) {
    if (size_ < cap_) {
      new (static_cast<Pointer>(buf_) + size_) A(std::move(data));
      size_++;
      return;
    }

    void* new_a = nullptr;
    try {
      new_a = Extend();
      new (static_cast<Pointer>(new_a) + size_) A(std::move(data));
    } catch (...) {
      std::destroy_n(static_cast<Pointer>(new_a), size_);
      operator delete(new_a);
      throw;
    }
    cap_ = (cap_ == 0) ? 1 : cap_ * 2;
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    size_++;
  }
  template <class... Args>
  void EmplaceBack(Args&&... ar) {
    if (size_ < cap_) {
      new (static_cast<Pointer>(buf_) + size_) A(std::forward<Args>(ar)...);
      size_++;
      return;
    }

    void* new_a = nullptr;
    try {
      new_a = Extend();
      new (static_cast<Pointer>(new_a) + size_) A(std::forward<Args>(ar)...);
    } catch (...) {
      std::uninitialized_move(static_cast<Pointer>(new_a), static_cast<Pointer>(new_a) + size_, static_cast<Pointer>(buf_));
      operator delete(new_a);
      throw;
    }
    cap_ = (cap_ == 0) ? 1 : cap_ * 2;
    std::destroy(begin(), end());
    operator delete(buf_);
    buf_ = new_a;
    size_++;
  }

  void PopBack() {
    if (size_ > 0) {
      std::destroy_at(static_cast<Pointer>(buf_) + size_ - 1);
      size_--;
    }
  }
  ConstIterator begin() const noexcept {  // NOLINT
    return static_cast<Pointer>(buf_);
  }
  Iterator begin() noexcept {  // NOLINT
    return static_cast<Pointer>(buf_);
  }
  ConstIterator end() const noexcept {  // NOLINT
    return static_cast<Pointer>(buf_) + size_;
  }
  Iterator end() noexcept {  // NOLINT
    return static_cast<Pointer>(buf_) + size_;
  }

  ConstIterator cbegin() const noexcept {  // NOLINT
    return static_cast<Pointer>(buf_);
  }
  ReverseIterator rbegin() noexcept {  // NOLINT
    return std::reverse_iterator(end());
  }
  ConstIterator cend() const noexcept {  // NOLINT
    return static_cast<Pointer>(buf_) + size_;
  }
  ConstReverseIterator rbegin() const noexcept {  // NOLINT
    return std::reverse_iterator(cend());
  }
  ReverseIterator rend() noexcept {  // NOLINT
    return std::reverse_iterator(begin());
  }
  ConstReverseIterator crbegin() const noexcept {  // NOLINT
    return std::reverse_iterator(cend());
  }
  ConstReverseIterator rend() const noexcept {  // NOLINT
    return std::reverse_iterator(cbegin());
  }
  ConstReverseIterator crend() const noexcept {  // NOLINT
    return std::reverse_iterator(cbegin());
  }
 private:
  void* buf_;
  size_t size_;
  size_t cap_;
};
template <typename A>
bool operator>=(const Vector<A>& start, const Vector<A>& finish) {
  bool ans = !(start < finish);
  return ans;
}
template <class A>
bool operator<(const Vector<A>& start, const Vector<A>& finish) {
  size_t last = std::min(start.size_, finish.size_), id = 0;
  for (id = 0; id < last; ++id) {
    if (start[id] < finish[id]) {
      return true;
    }
    if (start[id] > finish[id]) {
      return false;
    }
  }
  return static_cast<bool>(start.size_ < finish.size_);
}
template <typename A>
bool operator>(const Vector<A>& start, const Vector<A>& finish) {
  bool ans = (!(start < finish) && !(start == finish));
  return ans;
}
template <typename A>
bool operator!=(const Vector<A>& start, const Vector<A>& finish) {
  bool ans = !(start == finish);
  return ans;
}
template <typename A>
bool operator<=(const Vector<A>& start, const Vector<A>& finish) {
  bool ans = (start < finish || start == finish);
  return ans;
}
template <typename A>
bool operator==(const Vector<A>& start, const Vector<A>& finish) {
  size_t id = 0;
  if (start.size_ != finish.size_) {
    return false;
  }
  for (id = 0; id < start.size_; ++id) {
    if (start[id] != finish[id]) {
      return false;
    }
  }
  return true;
}