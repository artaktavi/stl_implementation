#include <cstring>
#include <iostream>
#include <type_traits>
#include <utility>
#include <compare>

template <typename T>
class Deque {
public:
  template <bool is_const>
  class it_template;
  using iterator = it_template<false>;
  using const_iterator = it_template<true>;

private:
  using smart_T_arg = std::conditional_t<std::is_class_v<T>, const T&, T>;
  template <bool is_const>
  using conditional_it = std::conditional_t<is_const, const_iterator, iterator>;

  static const size_t block_len_ = 32;  // size of T objects blocks

  size_t ptr_cap_ = 1;                  // capacity of ptr_array_
  size_t left_ptr_bound_ = 0;           // left not empty ptr index
  size_t right_ptr_bound_ = 0;          // right not empty ptr index
  size_t front_index_ = 0;              // index of first T in block (on left_ptr_bound_)
  size_t back_index_ = 0;               // 1 + index of last T in block (on right_ptr_bound_)
  size_t sz_ = 0;                       // count of T objects in Deque
  T** ptr_array_ = nullptr;             // array of pointers

  void shift_elements_right(iterator start) {
    T temp = *start;
    ++start;

    for (; start != end(); ++start) {
      std::swap(temp, *start);
    }

    push_back(temp);
  }

  void move_element_to_end(iterator start) {
    ++start;

    for (; start != end(); ++start) {
      std::swap(*(start - 1), *start);
    }
  }

  template <bool is_const>
  conditional_it<is_const> template_begin() const noexcept {
    return conditional_it<is_const>(ptr_array_ + left_ptr_bound_, front_index_);
  }

  template <bool is_const>
  conditional_it<is_const> template_end() const noexcept {
    if (back_index_ == block_len_) {
      return conditional_it<is_const>(ptr_array_ + right_ptr_bound_ + 1, 0ULL);
    }

    return conditional_it<is_const>(ptr_array_ + right_ptr_bound_, back_index_);
  }


  static void swap_deqs(Deque& first, Deque& second) noexcept {
    std::swap(first.ptr_cap_, second.ptr_cap_);
    std::swap(first.left_ptr_bound_, second.left_ptr_bound_);
    std::swap(first.right_ptr_bound_, second.right_ptr_bound_);
    std::swap(first.front_index_, second.front_index_);
    std::swap(first.back_index_, second.back_index_);
    std::swap(first.sz_, second.sz_);
    std::swap(first.ptr_array_, second.ptr_array_);
  }

  T** reserve_new_ptr_arr(T** old_arr, size_t& left_bound,
                          size_t& right_bound) {
    size_t old_size = right_bound - left_bound + 1;
    T** new_arr;
    new_arr = new T*[old_size * 3];
    memcpy(new_arr + old_size, old_arr + left_bound, old_size * sizeof(T*));
    left_bound = old_size;
    right_bound = old_size * 2 - 1;
    return new_arr;
  }

  void reserve_new_block(T** arr, size_t index) {
    arr[index] = reinterpret_cast<T*>(new char[block_len_ * sizeof(T)]);
  }

  void clean_all_blocks() const noexcept {
    clean_blocks_from_to(left_ptr_bound_, right_ptr_bound_, front_index_,
                         back_index_);
  }

  void clean_blocks_from_to(size_t ptr_from, size_t ptr_to, size_t from_ind,
                            size_t to_ind) const noexcept {
    if (ptr_from > ptr_to) {
      return;
    }

    size_t ind = from_ind;
    size_t ptr_ind;

    if (ptr_from == ptr_to) {

      for (; ind < to_ind; ++ind) {
        (ptr_array_[ptr_from] + ind)->~T();
      }

    } else {

      for (; ind < block_len_; ++ind) {
        (ptr_array_[ptr_from] + ind)->~T();
      }

      for (ptr_ind = ptr_from + 1; ptr_ind < ptr_to; ++ptr_ind) {
        ind = 0;

        for (; ind < block_len_; ++ind) {
          (ptr_array_[ptr_ind] + ind)->~T();
        }
      }

      for (ind = 0; ind < to_ind; ++ind) {
        (ptr_array_[ptr_to] + ind)->~T();
      }
    }

    for (ptr_ind = ptr_from; ptr_ind <= ptr_to; ++ptr_ind) {
      delete[] reinterpret_cast<char*>(ptr_array_[ptr_ind]);
    }
  }

  void clean_everything() const noexcept {
    clean_all_blocks();
    delete[] ptr_array_;
  }

  template <bool is_copy, typename... Args>
  void initialize_safely_with(size_t ptr_start, size_t start, size_t size, const Deque& other, Args&&... args) {
    size_t ptr_ind = ptr_start;
    size_t ind = start;

    try {
      try {
        reserve_new_block(ptr_array_, ptr_ind);
      } catch(...) {
        left_ptr_bound_ = right_ptr_bound_ + 1;
        throw;
      }

      size_t mini = (start + size) < block_len_ ? (start + size) : block_len_;

      for (; ind < mini; ++ind) {
        if constexpr (is_copy) {
          new (ptr_array_[ptr_ind] + ind) T(other.ptr_array_[ptr_ind][ind]);
        } else {
          new (ptr_array_[ptr_ind] + ind) T(std::forward<Args>(args)...);
        }
        --size;
      }

      for (++ptr_ind; ptr_ind <= right_ptr_bound_; ++ptr_ind) {
        try {
          reserve_new_block(ptr_array_, ptr_ind);
        } catch(...) {
          --ptr_ind;
          ind = block_len_;
          throw;
        }

        mini = size < block_len_ ? size : block_len_;

        for (ind = 0; ind < mini; ++ind) {
          if constexpr (is_copy) {
            new (ptr_array_[ptr_ind] + ind) T(other.ptr_array_[ptr_ind][ind]);
          } else {
            new (ptr_array_[ptr_ind] + ind) T(std::forward<Args>(args)...);
          }
          --size;
        }

      }
    } catch (...) {
      clean_blocks_from_to(left_ptr_bound_, ptr_ind, front_index_, ind);
      delete[] ptr_array_;
      throw;
    }
  }

  template <bool is_const>
  std::conditional_t<is_const, const T&, T&> template_at(size_t index) const {
    if (index >= sz_) {
      throw std::out_of_range("exception: Deque: at: out_of_range");
    }

    return const_cast<std::conditional_t<is_const, const T&, T&>>(operator[](index));
  }

public:
  ~Deque() noexcept {
    clean_everything();
  }

  Deque() : ptr_array_(new T*[1]) {
    try {
      reserve_new_block(ptr_array_, 0);
    } catch (...) {
      delete[] ptr_array_;
      throw;
    }
  }

  Deque(int count) : ptr_cap_(std::max(1ULL, static_cast<unsigned long>(count) / block_len_ +
                                             (static_cast<unsigned long>(count) % block_len_ != 0ULL ? 1ULL : 0ULL))),
                     right_ptr_bound_(ptr_cap_ - 1),
                     back_index_(count % block_len_ == 0 ? block_len_ : count % block_len_),
                     sz_(count),
                     ptr_array_(new T*[ptr_cap_]) {

    initialize_safely_with<false>(0, 0, count, *this);

  }

  Deque(const Deque& other) : ptr_cap_(other.ptr_cap_),
                              left_ptr_bound_(other.left_ptr_bound_),
                              right_ptr_bound_(other.right_ptr_bound_),
                              front_index_(other.front_index_),
                              back_index_(other.back_index_),
                              sz_(other.sz_),
                              ptr_array_(new T*[other.ptr_cap_]) {

    initialize_safely_with<true>(left_ptr_bound_, front_index_, sz_, other);

  }

  Deque(int count, const T& sample)
          : ptr_cap_(std::max(1ULL,
                              static_cast<unsigned long>(count) / block_len_ +
                              (static_cast<unsigned long>(count) % block_len_ != 0ULL ? 1ULL : 0ULL))),
            right_ptr_bound_(ptr_cap_ - 1),
            back_index_(count % block_len_ == 0 ? block_len_ : count % block_len_),
            sz_(count),
            ptr_array_(new T*[ptr_cap_]) {

    initialize_safely_with<false>(0, 0, count, *this, sample);

  }

  Deque& operator=(const Deque& other) {
    if (&other == this) {
      return *this;
    }
    Deque<T> new_deq(other);
    swap_deqs(new_deq, *this);
    return *this;
  }

  [[nodiscard]] size_t size() const noexcept {
    return sz_;
  }

  void push_back(smart_T_arg new_element) {
    size_t new_left_bnd = left_ptr_bound_;
    size_t new_right_bnd = right_ptr_bound_;
    T** new_arr = ptr_array_;

    if (right_ptr_bound_ == ptr_cap_ - 1 && back_index_ == block_len_) {
      new_arr = reserve_new_ptr_arr(ptr_array_, new_left_bnd, new_right_bnd);
    }

    bool block_added = back_index_ == block_len_;

    if (block_added) {
      ++new_right_bnd;

      try {
        reserve_new_block(new_arr, new_right_bnd);
      } catch (...) {

        if (new_arr != ptr_array_) {
          delete[] new_arr;
        }

        throw;
      }

      back_index_ = 0;
    }

    try {
      new (new_arr[new_right_bnd] + back_index_) T(new_element);
    } catch (...) {

      if (block_added) {
        delete[] reinterpret_cast<char*>(new_arr[new_right_bnd]);
        back_index_ = block_len_;
      }

      if (new_arr != ptr_array_) {
        delete[] new_arr;
      }

      throw;
    }

    if (new_arr != ptr_array_) {
      delete[] ptr_array_;
      ptr_cap_ = (right_ptr_bound_ - left_ptr_bound_ + 1) * 3;
      left_ptr_bound_ = new_left_bnd;
      ptr_array_ = new_arr;
    }

    right_ptr_bound_ = new_right_bnd;
    ++back_index_;
    ++sz_;
  }

  void push_front(smart_T_arg new_element) {
    size_t new_left_bnd = left_ptr_bound_;
    size_t new_right_bnd = right_ptr_bound_;
    T** new_arr = ptr_array_;

    if (left_ptr_bound_ == 0 && front_index_ == 0) {
      new_arr = reserve_new_ptr_arr(ptr_array_, new_left_bnd, new_right_bnd);
    }

    bool block_added = front_index_ == 0;

    if (block_added) {
      --new_left_bnd;

      try {
        reserve_new_block(new_arr, new_left_bnd);
      } catch (...) {

        if (new_arr != ptr_array_) {
          delete[] new_arr;
        }

        throw;
      }

      front_index_ = block_len_;
    }

    --front_index_;

    try {
      new (new_arr[new_left_bnd] + front_index_) T(new_element);
    } catch (...) {
      ++front_index_;

      if (block_added) {
        delete[] reinterpret_cast<char*>(new_arr[new_left_bnd]);
        front_index_ = 0;
      }

      if (new_arr != ptr_array_) {
        delete[] new_arr;
      }

      throw;
    }

    if (new_arr != ptr_array_) {
      delete[] ptr_array_;
      ptr_cap_ = (right_ptr_bound_ - left_ptr_bound_ + 1) * 3;
      ptr_array_ = new_arr;
      right_ptr_bound_ = new_right_bnd;
    }

    left_ptr_bound_ = new_left_bnd;
    ++sz_;
  }

  void pop_back() {
    if (back_index_ == 0) {
      delete[] reinterpret_cast<char*>(ptr_array_[right_ptr_bound_]);
      back_index_ = block_len_;
      --right_ptr_bound_;
    }

    --back_index_;
    (ptr_array_[right_ptr_bound_] + back_index_)->~T();
    --sz_;
  }

  void pop_front() {
    (ptr_array_[left_ptr_bound_] + front_index_)->~T();
    ++front_index_;

    if (front_index_ == block_len_) {
      delete[] reinterpret_cast<char*>(ptr_array_[left_ptr_bound_]);
      front_index_ = 0;
      ++left_ptr_bound_;
    }

    --sz_;
  }

  void insert(iterator iter, const T& value) {
    if (iter == end()) {
      push_back(value);
      return;
    }

    shift_elements_right(iter);
    *iter = value;
  }

  void erase(iterator iter) {
    move_element_to_end(iter);
    pop_back();
  }

  const T& operator[](size_t index) const noexcept {
    const_iterator it_begin = cbegin();
    return *(it_begin + index);
  }

  T& operator[](size_t index) noexcept {
    iterator it_begin = begin();
    return *(it_begin + index);
  }

  const T& at(size_t index) const {
    return template_at<true>(index);
  }

  T& at(size_t index) {
    return template_at<false>(index);
  }

  template <bool is_const>
  class it_template {

    using smart_T = std::conditional_t<is_const, const T, T>;
    using smart_T_ref = std::conditional_t<is_const, const T&, T&>;
    using smart_T_ptr = std::conditional_t<is_const, const T*, T*>;

    T** block_ptr_;
    T* element_ptr_;

  public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = int;
    using value_type = smart_T;
    using pointer = smart_T_ptr;
    using reference = smart_T_ref;

    it_template(const it_template& other) = default;

    it_template(T** ptr, long long ind) : block_ptr_(ptr), element_ptr_(*ptr + ind) {}

    ~it_template() = default;

    operator it_template<true>() const noexcept {
      return it_template<true>(block_ptr_, element_ptr_);
    }

    it_template& operator++() noexcept {
      return operator+=(1);
    }

    it_template operator++(int) noexcept {
      it_template new_it(*this);
      ++(*this);
      return new_it;
    }

    it_template& operator--() noexcept {
      return operator-=(1);
    }

    it_template operator--(int) noexcept {
      it_template new_it(*this);
      --(*this);
      return new_it;
    }

    it_template& operator+=(int n) noexcept {
      if (n < 0) {
        *this -= -n;
        return *this;
      }

      long long temp_ind = element_ptr_ - *block_ptr_;
      temp_ind += n;
      block_ptr_ += temp_ind / block_len_;
      element_ptr_ = *block_ptr_ + (temp_ind % block_len_);
      return *this;
    }

    it_template operator+(int n) const noexcept {
      it_template new_it(*this);
      new_it += n;
      return new_it;
    }

    it_template& operator-=(int n) noexcept {
      if (n < 0) {
        *this += -n;
        return *this;
      }

      long long temp_ind = element_ptr_ - *block_ptr_;
      temp_ind -= n;

      if (temp_ind < 0) {
        block_ptr_ +=
                (temp_ind + 1LL) / static_cast<long long>(block_len_) - 1;
        long long new_shift = static_cast<long long>(block_len_) +
                               (temp_ind % static_cast<long long>(block_len_));

        if (new_shift == block_len_) {
          element_ptr_ = *block_ptr_;
        } else {
          element_ptr_ = *block_ptr_ + new_shift;
        }
      } else {
        element_ptr_ = *block_ptr_ + temp_ind;
      }

      return *this;
    }

    it_template operator-(int n) const noexcept {
      it_template new_it(*this);
      new_it -= n;
      return new_it;
    }

    bool operator==(it_template other) const noexcept {
      return block_ptr_ == other.block_ptr_ &&
             element_ptr_ == other.element_ptr_;
    }

    auto operator<=>(const it_template&) const = default;

    int operator-(it_template other) const noexcept {
      int diff = (block_ptr_ - other.block_ptr_) * block_len_ + (element_ptr_ - *block_ptr_) -
                 (other.element_ptr_ - *other.block_ptr_);

      return diff;
    }

    smart_T_ref operator*() const noexcept {
      return *element_ptr_;
    }

    smart_T_ptr operator->() const noexcept {
      return element_ptr_;
    }
  };

  iterator begin() noexcept {
    return template_begin<false>();
  }

  iterator end() noexcept {
    return template_end<false>();
  }

  const_iterator cbegin() const noexcept {
    return template_begin<true>();
  }

  const_iterator cend() const noexcept {
    return template_end<true>();
  }

  const_iterator begin() const noexcept {
    return cbegin();
  }

  const_iterator end() const noexcept {
    return cend();
  }

  std::reverse_iterator<iterator> rbegin() noexcept {
    return std::reverse_iterator(end());
  }

  std::reverse_iterator<iterator> rend() noexcept {
    return std::reverse_iterator(begin());
  }

  std::reverse_iterator<const_iterator> rbegin() const noexcept {
    return std::reverse_iterator(end());
  }

  std::reverse_iterator<const_iterator> rend() const noexcept {
    return std::reverse_iterator(begin());
  }

  std::reverse_iterator<const_iterator> crbegin() const noexcept {
    return std::reverse_iterator(cend());
  }

  std::reverse_iterator<const_iterator> crend() const noexcept {
    return std::reverse_iterator(cbegin());
  }
};
