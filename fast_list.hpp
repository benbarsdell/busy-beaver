/*
 * Copyright (c) 2019, Ben Barsdell. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <cassert>
#include <memory>
#include <vector>

// Same API as std::list, but all memory is stored in a single std::vector.
// Note that unlike std::list, this will potentially invalidate iterators after
// a move-construct/assign (because iterators hold a reference to their original
// FastList object).
template <typename T, class Allocator = std::allocator<T>,
          typename SizeType = int>  // TODO: SizeType being signed is hacky.
class FastList {
 public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef SizeType size_type;
  typedef SizeType difference_type;
  typedef Allocator allocator_type;

 private:
  class Node {
    typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type
        raw;

   public:
    size_type next;
    size_type prev;
    pointer value_ptr() { return reinterpret_cast<pointer>(&raw); }
    const_pointer value_ptr() const {
      return reinterpret_cast<const_pointer>(&raw);
    }
  };

  typedef typename std::allocator_traits<Allocator>::template rebind_alloc<Node>
      node_allocator_type;

  template <typename ContainerType, typename NodeType, typename Pointer,
            typename Reference>
  class Iterator {
   public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef typename FastList::value_type value_type;
    typedef Pointer pointer;
    typedef Reference reference;
    typedef typename FastList::size_type size_type;
    typedef typename FastList::difference_type difference_type;

   private:
    friend class FastList;
    Iterator(ContainerType* container, size_type index)
        : container_(container), index_(index) {}
    NodeType& node() const {
      assert(0 <= index_ && index_ < (int)container_->nodes_.size());
      return container_->nodes_[index_];
    }
    ContainerType* container_;
    size_type index_;

   public:
    // This allows implicit conversion from iterator to const_iterator.
    template <typename C, typename N, typename P, typename R>
    Iterator(const Iterator<C, N, P, R>& other)
        : container_(other.container_), index_(other.index_) {}
    Iterator& operator++() {
      index_ = node().next;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    Iterator& operator--() {
      index_ = node().prev;
      return *this;
    }
    Iterator operator--(int) {
      Iterator tmp = *this;
      --(*this);
      return tmp;
    }
    reference operator*() const {
      assert(index_ != 0);
      assert(index_ < (int)container_->nodes_.size());
      return *node().value_ptr();
    }
    pointer operator->() const {
      assert(index_ != 0);
      assert(index_ < (int)container_->nodes_.size());
      return node().value_ptr();
    }
    template <typename C, typename N, typename P, typename R>
    bool operator==(const Iterator<C, N, P, R>& other) const {
      return container_ == other.container_ && index_ == other.index_;
    }
    template <typename C, typename N, typename P, typename R>
    bool operator!=(const Iterator<C, N, P, R>& other) const {
      return !(*this == other);
    }
  };

 public:
  typedef Iterator<FastList, Node, pointer, reference> iterator;
  typedef Iterator<const FastList, const Node, const_pointer, const_reference>
      const_iterator;
  FastList() : free_head_(-1), size_(0) {
    // Add sentry node that acts as the "end" of the list. This node's
    // next/prev refers to the first/last actual element in the list (or
    // to the sentry node itself if the list is empty).
    nodes_.emplace_back();
    nodes_.back().next = 0;
    nodes_.back().prev = 0;
  }
  ~FastList() {
    while (!empty()) pop_back();
  }
  FastList(const FastList& other) : FastList() {
    for (const auto& item : other) emplace_back(item);
  }
  FastList& operator=(const FastList& other) {
    while (!empty()) pop_back();
    for (const auto& item : other) emplace_back(item);
    return *this;
  }

  // TODO: Try to clean these up a bit (dedupe the code).
  FastList(FastList&& other)
      : nodes_(std::move(other.nodes_)),
        free_head_(other.free_head_),
        size_(other.size_) {
    other.free_head_ = -1;
    other.size_ = 0;
    other.nodes_.clear();
    other.nodes_.emplace_back();
    other.nodes_.back().next = 0;
    other.nodes_.back().prev = 0;
  }
  FastList& operator=(FastList&& other) {
    while (!empty()) pop_back();
    nodes_ = std::move(other.nodes_);
    free_head_ = other.free_head_;
    size_ = other.size_;
    other.free_head_ = -1;
    other.size_ = 0;
    other.nodes_.clear();
    other.nodes_.emplace_back();
    other.nodes_.back().next = 0;
    other.nodes_.back().prev = 0;
    return *this;
  }

  void clear() {
    free_head_ = -1;
    size_ = 0;
    nodes_.clear();
    nodes_.emplace_back();
    nodes_.back().next = 0;
    nodes_.back().prev = 0;
  }

  template <typename... Args>
  iterator emplace(iterator it, Args&&... args) {
    size_type index;
    if (free_head_ != -1) {
      index = free_head_;
      free_head_ = nodes_[free_head_].next;
    } else {
      index = nodes_.size();
      nodes_.emplace_back();
    }
    size_type prev_index = nodes_[it.index_].prev;
    nodes_[index].next = it.index_;
    nodes_[index].prev = prev_index;
    // TODO: Add support for stateful allocators.
    node_allocator_type allocator;
    std::allocator_traits<node_allocator_type>::construct(
        allocator, nodes_[index].value_ptr(), std::forward<Args>(args)...);
    nodes_[it.index_].prev = index;
    nodes_[prev_index].next = index;
    ++size_;
    return iterator(this, index);
  }
  iterator insert(iterator it, const_reference value) {
    return emplace(it, value);
  }
  iterator erase(iterator it) {
    assert(!empty());
    assert(it != end());
    size_type prev_index = nodes_[it.index_].prev;
    iterator next_it = std::next(it);
    nodes_[it.index_].value_ptr()->~value_type();
    nodes_[it.index_].next = free_head_;
    free_head_ = it.index_;
    nodes_[next_it.index_].prev = prev_index;
    nodes_[prev_index].next = next_it.index_;
    --size_;
    return next_it;
  }
  iterator begin() { return std::next(iterator(this, 0)); }
  iterator end() { return iterator(this, 0); }
  const_iterator begin() const { return std::next(const_iterator(this, 0)); }
  const_iterator end() const { return const_iterator(this, 0); }
  void push_front(const T& value) { insert(begin(), value); }
  void push_back(const T& value) { insert(end(), value); }
  void pop_front() { erase(begin()); }
  void pop_back() { erase(std::prev(end())); }
  size_type size() const { return size_; }
  bool empty() const { return size_ == 0; }
  reference front() { return *begin(); }
  const_reference front() const { return *begin(); }
  reference back() { return *std::prev(end()); }
  const_reference back() const { return *std::prev(end()); }
  template <typename... Args>
  void emplace_front(Args&&... args) {
    emplace(begin(), std::forward<Args>(args)...);
  }
  template <typename... Args>
  void emplace_back(Args&&... args) {
    emplace(end(), std::forward<Args>(args)...);
  }

 private:
  std::vector<Node, node_allocator_type> nodes_;
  size_type free_head_;
  size_type size_;
};
