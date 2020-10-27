#pragma once
#include <iterator>
#include <type_traits>

namespace intrusive {
struct default_tag;

template<typename Tag = default_tag>
struct list_element {
  void unlink() noexcept {
    if (next != nullptr) {
      next->prev = prev;
    }
    if (prev != nullptr) {
      prev->next = next;
    }
    next = nullptr;
    prev = nullptr;
  }

  bool is_linked() const noexcept {
    return next != nullptr || prev != nullptr;
  }

  ~list_element() {
    unlink();
  }

  list_element* next = nullptr;
  list_element* prev = nullptr;
};

template<typename T, typename Tag = default_tag>
struct list {
  static_assert(std::is_convertible_v<T&, list_element<Tag>&>,
                "value type is not convertible to list_element");

  template<bool IsConst>
  struct list_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::remove_const_t<T>;
    using difference_type = std::ptrdiff_t;
    using pointer = std::conditional_t<IsConst,
        const value_type,
        value_type>*;
    using reference = std::conditional_t<IsConst,
        const value_type,
        value_type>&;

    list_iterator() = default;
    list_iterator(list_iterator const&) noexcept = default;
    list_iterator& operator=(list_iterator const& other) noexcept = default;

    template<bool OtherConst, class = std::enable_if_t<IsConst && !OtherConst>>
    list_iterator(list_iterator<OtherConst> const& other) noexcept
        : element(other.element) {}

    template<bool OtherConst, class = std::enable_if_t<IsConst && !OtherConst>>
    list_iterator& operator=(list_iterator<OtherConst> const& other) noexcept {
      element = other.element;
      return *this;
    }

    list_iterator& operator++() {
      element = element->next;
      return *this;
    }

    list_iterator operator++(int) {
      list_iterator res_it = *this;
      ++(*this);
      return res_it;
    }

    list_iterator& operator--() {
      element = element->prev;
      return *this;
    }

    list_iterator operator--(int) {
      list_iterator res_it = *this;
      --(*this);
      return res_it;
    }

    bool operator==(list_iterator const& other) const noexcept {
      return element == other.element;
    }

    bool operator!=(list_iterator const& other) const noexcept {
      return element != other.element;
    }

    reference operator*() const noexcept {
      return *static_cast<pointer>(element);
    }

    pointer operator->() const noexcept {
      return static_cast<pointer>(element);
    }

  private:
    using ptr_type = std::conditional_t<IsConst,
        const list_element<Tag>,
        list_element<Tag>>;
    explicit list_iterator(ptr_type* element) noexcept
        : element(element) {}

  private:
    template<bool IsOtherConst> friend
    struct list_iterator;
    friend struct list;

    ptr_type* element = nullptr;
  };

  using iterator = list_iterator<false>;
  using const_iterator = list_iterator<true>;

  list() noexcept {
    null_node.prev = &null_node;
    null_node.next = &null_node;
  }

  void swap(list& other) noexcept {
    using std::swap;
    swap(null_node.prev, other.null_node.prev);
    swap(null_node.prev->next, other.null_node.prev->next);
    swap(null_node.next, other.null_node.next);
    swap(null_node.next->prev, other.null_node.next->prev);
  }

  list(list const&) = delete;
  list(list&& other) noexcept
      : list() {
    swap(other);
  }

  list& operator=(list const&) = delete;
  list& operator=(list&& other) noexcept {
    if (this != &other) {
      swap(other);
      other.clear();
    }
    return *this;
  }

  ~list() {
    clear();
  }

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  }

  void push_back(T& element) noexcept {
    insert(end(), element);
  }

  void pop_back() noexcept {
    static_cast<list_element<Tag>&>(back()).unlink();
  }

  T& back() noexcept {
    return *(static_cast<T*>(null_node.prev));
  }

  T const& back() const noexcept {
    return *(static_cast<T const*>(null_node.prev));
  }

  void push_front(T& element) noexcept {
    insert(begin(), element);
  }

  void pop_front() noexcept {
    static_cast<list_element<Tag>&>(front()).unlink();
  }

  T& front() noexcept {
    return *(static_cast<T*>(null_node.next));
  }

  T const& front() const noexcept {
    return *(static_cast<T*>(null_node.next));
  }

  bool empty() const noexcept {
    return null_node.next == &null_node && null_node.prev == &null_node;
  }

  iterator begin() noexcept {
    return iterator(null_node.next);
  }

  const_iterator begin() const noexcept {
    return const_iterator(null_node.next);
  }

  iterator end() noexcept {
    return iterator(&null_node);
  }

  const_iterator end() const noexcept {
    return const_iterator(&null_node);
  }

  iterator insert(const_iterator pos, T& element) noexcept {
    auto element_ptr = static_cast<list_element<Tag>*>(&element);
    element_ptr->unlink();
    list_element<Tag>* p = iterator_to_ptr(pos);
    iterator_to_ptr(--pos)->next = &element;
    element_ptr->prev = iterator_to_ptr(pos);
    element_ptr->next = p;
    p->prev = &element;
    return iterator(&element);
  }

  iterator erase(const_iterator pos) noexcept {
    list_element<Tag>* p = iterator_to_ptr(pos);
    iterator next(p);
    next++;
    p->unlink();
    return next;
  }

  void splice(const_iterator pos, list&, const_iterator first,
              const_iterator last) noexcept {
    if (first != last) {
      list_element<Tag>* f = iterator_to_ptr(first);
      list_element<Tag>* p = iterator_to_ptr(pos);
      list_element<Tag>* l = iterator_to_ptr(last);

      std::swap(last->prev->next, pos->prev->next);
      std::swap(pos->prev->next, first->prev->next);

      std::swap(l->prev, f->prev);
      std::swap(f->prev, p->prev);
    }
  }

  iterator as_iterator(T& element) noexcept {
    return iterator(static_cast<list_element<Tag>*>(&element));
  }

  const_iterator as_iterator(T& element) const noexcept {
    return const_iterator(static_cast<list_element<Tag>*>(&element));
  }

private:
  static list_element<Tag>* iterator_to_ptr(const_iterator p) noexcept {
    return const_cast<list_element<Tag>*>(static_cast<list_element<Tag> const*>(&*p));
  }

private:
  list_element<Tag> null_node;
};
} // namespace intrusive