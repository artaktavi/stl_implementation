#include <iostream>
#include <memory>
#include <type_traits>

//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////    StackStorage    ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template <size_t N>
class StackStorage {
    char mem_[N];
    size_t access_ = 0;

public:
    StackStorage() = default;

    StackStorage(const StackStorage<N>&) = delete;

    ~StackStorage() = default;

    char* takeMem(size_t bytes_count, size_t alignment) noexcept {
      size_t memory_left = N - access_;
      void* temp = static_cast<void*>(mem_ + access_);
      char* res_ptr = static_cast<char*>(
              std::align(alignment, bytes_count, temp, memory_left));
      access_ = (res_ptr - mem_) + bytes_count;
      return res_ptr;
    }
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////    StackAllocator    //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template <typename T, size_t N>
class StackAllocator {
    StackStorage<N>* storage_ = nullptr;

public:
    using value_type = T;

    StackAllocator() = delete;

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& alloc)
            : storage_(alloc.getStorage()) {}

    StackAllocator(StackStorage<N>& storage) : storage_(&storage) {}

    ~StackAllocator() = default;

    StackAllocator& operator=(const StackAllocator& other) noexcept = default;

    T* allocate(size_t count) const {
      T* ptr =
              reinterpret_cast<T*>(storage_->takeMem(count * sizeof(T), alignof(T)));
      if (ptr == nullptr) {
        throw std::bad_alloc();
      }
      return ptr;
    }

    void deallocate(T*, size_t) const noexcept {}

    StackStorage<N>* getStorage() const noexcept {
      return storage_;
    }

    template <typename U, size_t M>
    bool operator==(const StackAllocator<U, M>& other) {
      return storage_ == other.storage_;
    }

    template <typename U, size_t M>
    bool operator!=(const StackAllocator<U, M>& other) {
      return storage_ != other.storage_;
    }

    template <typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };
};

//////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////    List    ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template <typename T, class Allocator = std::allocator<T>>
class List {
    struct BasicNode;
    struct Node;

    using AllocTraits = std::allocator_traits<Allocator>;
    using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
    using NodeTraits = std::allocator_traits<NodeAllocator>;
    using smart_T = std::conditional_t<std::is_class_v<T>, const T&, T>;

    struct BasicNode {
        BasicNode* next;
        BasicNode* prev;
        BasicNode(BasicNode* next, BasicNode* prev) : next(next), prev(prev) {}
    };

    struct Node : BasicNode {
        T value;

        template <typename... Args>
        Node(BasicNode* next, BasicNode* prev, Args&&... args)
                : BasicNode(next, prev), value(std::forward<Args>(args)...) {}
    };

    [[no_unique_address]] NodeAllocator alloc_;
    size_t sz_ = 0;
    BasicNode root_ = {&root_, &root_};

    template <bool is_const>
    class template_it {
        BasicNode* node_;

        using smart_T = std::conditional_t<is_const, const T, T>;
        using smart_T_ref = std::conditional_t<is_const, const T&, T&>;
        using smart_T_ptr = std::conditional_t<is_const, const T*, T*>;

        friend class List;

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = int;
        using value_type = smart_T;
        using pointer = smart_T_ptr;
        using reference = smart_T_ref;

        template_it(BasicNode* node) : node_(node) {}

        template_it(const template_it& other) = default;

        ~template_it() = default;

        operator template_it<true>() const noexcept {
          return template_it<true>(node_);
        }

        smart_T_ref operator*() const noexcept {
          return static_cast<Node*>(node_)->value;
        }

        smart_T_ptr operator->() const noexcept {
          return &(static_cast<Node*>(node_)->value);
        }

        template_it& operator++() noexcept {
          node_ = node_->next;
          return *this;
        }

        template_it operator++(int) noexcept {
          template_it old(*this);
          ++(*this);
          return old;
        }

        template_it& operator--() noexcept {
          node_ = node_->prev;
          return *this;
        }

        template_it operator--(int) noexcept {
          template_it old(*this);
          --(*this);
          return old;
        }

        bool operator==(template_it other) const noexcept {
          return other.node_ == node_;
        }

        bool operator!=(template_it other) const noexcept {
          return other.node_ != node_;
        }

        template_it& operator=(template_it other) noexcept {
          node_ = other.node_;
          return *this;
        }
    };

    template <typename... Args>
    Node* create_node(BasicNode* next, BasicNode* prev, Args&&... args) {
      Node* new_node(NodeTraits::allocate(alloc_, 1));
      try {
        NodeTraits::construct(alloc_, new_node, next, prev,
                              std::forward<Args>(args)...);
      } catch (...) {
        NodeTraits::deallocate(alloc_, new_node, 1);
        throw;
      }
      return new_node;
    }

    template <typename... Args>
    void safe_initialize(size_t count, Args&&... args) {
      try {

        for (size_t created = 0; created < count; ++created) {
          emplace_back(std::forward<Args>(args)...);
        }

      } catch (...) {

        for (; sz_ > 0;) {
          pop_back();
        }

        throw;
      }
    }

    void delete_all() {
      for (; sz_ > 0;) {
        pop_back();
      }
    }

    void add_from(const List& other, template_it<true> it) {
      size_t added = 0;
      try {
        const_iterator other_cend = other.cend();

        for (; it != other_cend; ++it) {
          emplace_back(*it);
          ++added;
        }

      } catch (...) {

        for (; added > 0; --added) {
          pop_back();
        }

        throw;
      }
    }

    void assign_from(const List& other) {
      template_it<false> it_this = begin();
      template_it<true> it_other = other.begin();

      while (it_this != end() && it_other != other.end()) {
        *it_this = *it_other;
        ++it_other;
        ++it_this;
      }

      if (it_this == end()) {

        add_from(other, it_other);

      } else {

        while (it_this != --end()) {
          pop_back();
        }

        pop_back();
      }
    }

public:
    using iterator = template_it<false>;
    using const_iterator = template_it<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    iterator begin() noexcept {
      return iterator(root_.next);
    }

    iterator end() noexcept {
      return iterator(const_cast<BasicNode*>(&root_));
    }

    const_iterator begin() const noexcept {
      return const_iterator(root_.next);
    }

    const_iterator end() const noexcept {
      return const_iterator(const_cast<BasicNode*>(&root_));
    }

    const_iterator cbegin() const noexcept {
      return const_iterator(root_.next);
    }

    const_iterator cend() const noexcept {
      return const_iterator(const_cast<BasicNode*>(&root_));
    }

    reverse_iterator rbegin() noexcept {
      return std::reverse_iterator<iterator>(end());
    }

    reverse_iterator rend() noexcept {
      return std::reverse_iterator<iterator>(begin());
    }

    const_reverse_iterator rbegin() const noexcept {
      return std::reverse_iterator<const_iterator>(cend());
    }

    const_reverse_iterator rend() const noexcept {
      return std::reverse_iterator<const_iterator>(cbegin());
    }

    const_reverse_iterator crbegin() const noexcept {
      return std::reverse_iterator<const_iterator>(cend());
    }

    const_reverse_iterator crend() const noexcept {
      return std::reverse_iterator<const_iterator>(cbegin());
    }

    List(Allocator alloc) : alloc_(alloc) {}

    List(size_t count = 0ULL) {
      safe_initialize(count);
    }

    List(size_t count, const T& object, Allocator alloc = Allocator())
            : alloc_(alloc) {
      safe_initialize(count, object);
    }

    List(size_t count, Allocator alloc) : alloc_(alloc) {
      safe_initialize(count);
    }

    List(const List& other)
            : alloc_(NodeTraits::select_on_container_copy_construction(other.alloc_)) {
      add_from(other, other.cbegin());
    }

    List& operator=(const List& other) {
      if constexpr (NodeTraits::propagate_on_container_copy_assignment::value) {

        if (other.alloc_ != alloc_) {
          delete_all();
          alloc_ = other.alloc_;
        }

      }
      assign_from(other);
      return *this;
    }

    ~List() {
      delete_all();
    }

    Allocator get_allocator() const noexcept {
      return Allocator(alloc_);
    }

    [[nodiscard]] size_t size() const noexcept {
      return sz_;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
      Node* new_node =
              create_node(&root_, root_.prev, std::forward<Args>(args)...);
      root_.prev->next = new_node;
      root_.prev = new_node;
      ++sz_;
    }

    void push_back(smart_T object) {
      Node* new_node = create_node(&root_, root_.prev, object);
      root_.prev->next = new_node;
      root_.prev = new_node;
      ++sz_;
    }

    void push_front(smart_T object) {
      Node* new_node = create_node(root_.next, &root_, object);
      root_.next->prev = new_node;
      root_.next = new_node;
      ++sz_;
    }

    void pop_back() {
      BasicNode* prev_node = root_.prev->prev;
      NodeTraits::destroy(alloc_, static_cast<Node*>(root_.prev));
      NodeTraits::deallocate(alloc_, static_cast<Node*>(root_.prev), 1);
      prev_node->next = &root_;
      root_.prev = prev_node;
      --sz_;
    }

    void pop_front() {
      BasicNode* second_node = root_.next->next;
      NodeTraits::destroy(alloc_, static_cast<Node*>(root_.next));
      NodeTraits::deallocate(alloc_, static_cast<Node*>(root_.next), 1);
      second_node->prev = &root_;
      root_.next = second_node;
      --sz_;
    }

    void insert(const_iterator it, smart_T object) {
      BasicNode* new_node = create_node(it.node_, it.node_->prev, object);
      it.node_->prev->next = new_node;
      it.node_->prev = new_node;
      ++sz_;
    }

    void erase(const_iterator it) {
      BasicNode* nxt = it.node_->next;
      BasicNode* prv = it.node_->prev;
      NodeTraits::destroy(alloc_, static_cast<Node*>(it.node_));
      NodeTraits::deallocate(alloc_, static_cast<Node*>(it.node_), 1);
      nxt->prev = prv;
      prv->next = nxt;
      --sz_;
    }
};
