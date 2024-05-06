#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iterator>
#include <tuple>
#include <type_traits>

template <typename Key, typename Value, typename Hash = std::hash<Key>,
        typename Equal = std::equal_to<Key>,
        typename Allocator = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, Value>;

private:
    struct BasicNode {
        BasicNode* next;
    };

    struct Node : BasicNode {
        size_t hash_ind;
        NodeType keyval;
    };

    struct BidirectNode : BasicNode {
        BasicNode* prev;
        BidirectNode(BasicNode* next, BasicNode* prev)
                : BasicNode(next), prev(prev) {}
    };

    using AllocTraits = std::allocator_traits<Allocator>;
    using NodeAllocator = typename AllocTraits::template rebind_alloc<Node>;
    using NodeTraits = std::allocator_traits<NodeAllocator>;
    template <bool is_const>
    using ConditionalNodeType =
            std::conditional_t<is_const, const NodeType, NodeType>;

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////   ITERATOR   //////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <bool is_const>
    class template_it {
        BasicNode* node_;

        friend class UnorderedMap;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        using value_type = NodeType;
        using pointer = NodeType*;
        using reference = NodeType&;

        template_it(BasicNode* node) : node_(node) {}
        template_it(const template_it& other) = default;
        template_it() = default;
        ~template_it() = default;

        operator template_it<true>() const noexcept {
          return template_it<true>(node_);
        }

        ConditionalNodeType<is_const>& operator*() const noexcept {
          return static_cast<Node*>(node_)->keyval;
        }

        ConditionalNodeType<is_const>* operator->() const noexcept {
          return &(static_cast<Node*>(node_)->keyval);
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

public:
    using iterator = template_it<false>;
    using const_iterator = template_it<true>;

private:
    template <bool is_const>
    using conditional_iterator =
            std::conditional_t<is_const, const_iterator, iterator>;

    //////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////   FORWARDLIST   //////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    struct ForwardList {
        [[no_unique_address]] NodeAllocator alloc_;
        size_t sz = 0;
        BidirectNode root = {&root, &root};

        template <typename... Args>
        Node*
        create_node(BasicNode* next_node, size_t hash, Args&&... args) noexcept(
        noexcept(NodeTraits::allocate(
                std::declval<NodeAllocator&>(),
                std::declval<
                        size_t&>())) && noexcept(NodeTraits::
        construct(
                std::declval<NodeAllocator&>(),
                std::declval<NodeType*&>(),
                std::forward<Args>(
                        args)...))) {
          Node* new_node(NodeTraits::allocate(alloc_, 1));
          try {
            new_node->next = next_node;
            new_node->hash_ind = hash;
            NodeTraits::construct(alloc_,
                                  static_cast<NodeType*>(&(new_node->keyval)),
                                  std::forward<Args>(args)...);
          } catch (...) {
            NodeTraits::deallocate(alloc_, new_node, 1);
            throw;
          }
          return new_node;
        }

        void delete_node(Node* node) noexcept(noexcept(NodeTraits::destroy(
                std::declval<NodeAllocator&>(), std::declval<Node*&>()))) {
          NodeTraits::destroy(alloc_, node);
          NodeTraits::deallocate(alloc_, node, 1);
        }

        void delete_all() noexcept(noexcept(pop_front())) {
          for (; sz > 0;) {
            pop_front();
          }
        }

        void detach_nodes() noexcept {
          root.prev = &root;
          root.next = &root;
          sz = 0;
        }

        void add_from(const ForwardList& other, const_iterator it) noexcept(
        noexcept(emplace_back(std::declval<const NodeType&>()))) {
          try {
            const_iterator other_end = other.end();
            for (; it != other_end; ++it) {
              emplace_back(*it);
            }
          } catch (...) {
            std::cerr << "add_from: RECOVERY IS IMPOSSIBLE" << std::endl;
            throw;
          }
        }

        void assign_from(const ForwardList& other) noexcept(noexcept(add_from(
                std::declval<const ForwardList&>(),
                std::declval<
                        const_iterator>())) && noexcept(erase_after(std::
                                                                    declval<
                BasicNode*>()))) {
          iterator it_this = end();
          const_iterator it_other = other.end();
          while (it_this.node_->next != &root &&
                 it_other.node_->next != &(other.root)) {
            ++it_other;
            ++it_this;
            *it_this = *it_other;
          }
          if (it_this.node_->next == root) {
            add_from(other, ++it_other);
          } else {
            while (it_this.node_.next != root) {
              erase_after(it_this.node_);
            }
          }
        }

        void swap_nodes(ForwardList& other) noexcept {
          if (other.root.prev != &(other.root)) {
            other.root.prev->next = &root;
          }
          if (root.prev != &root) {
            root.prev->next = &(other.root);
          }
          BasicNode* nxt = root.next;
          root.next = (other.root.next != &(other.root)) ? other.root.next : &root;
          other.root.next = (nxt != &root) ? nxt : &(other.root);
          BasicNode* prv = root.prev;
          root.prev = (other.root.prev != &(other.root)) ? other.root.prev : &root;
          other.root.prev = (prv != &root) ? prv : &(other.root);
          std::swap(sz, other.sz);
        }

    public:
        iterator begin() noexcept { return iterator(root.next); }

        iterator end() noexcept { return iterator(static_cast<BasicNode*>(&root)); }

        const_iterator begin() const noexcept { return const_iterator(root.next); }

        const_iterator end() const noexcept {
          return const_iterator(
                  const_cast<BasicNode*>(static_cast<const BasicNode*>(&root)));
        }

        const_iterator cbegin() const noexcept { return const_iterator(root.next); }

        const_iterator cend() const noexcept {
          return const_iterator(const_cast<BidirectNode*>(&root));
        }
        ForwardList() = default;

        ForwardList(Allocator alloc) noexcept : alloc_(alloc) {}

        ForwardList(const ForwardList& other) noexcept(noexcept(
                add_from(std::declval<ForwardList&>(), std::declval<iterator&>())))
                : alloc_(
                NodeTraits::select_on_container_copy_construction(other.alloc_)) {
          add_from(other, other.cbegin());
        }

        ForwardList(ForwardList&& other) noexcept
                : alloc_(std::move(other.alloc_)) {
          swap_nodes(other);
        }

        void swap(ForwardList& other) noexcept {
          if constexpr (NodeTraits::propagate_on_container_swap::value) {
            if (alloc_ != other.alloc_) {
              alloc_.swap(other.alloc_);
            }
          } else {
            assert(alloc_ == other.alloc_);
            // UB if propagate_on_container_swap is false and allocators are not equal
            // https://en.cppreference.com/w/cpp/named_req/Allocator
          }
          swap_nodes(other);
        }

        ForwardList& operator=(const ForwardList& other) noexcept(noexcept(
                                                                          delete_all()) && noexcept(assign_from(std::declval<ForwardList&>()))) {
          if constexpr (NodeTraits::propagate_on_container_copy_assignment::value) {
            if (other.alloc_ != alloc_) {
              delete_all();
              alloc_ = other.alloc_;
            }
          }
          assign_from(other);
          return *this;
        }

        ForwardList& operator=(ForwardList&& other) noexcept(
        noexcept(delete_all())) {
          delete_all();
          if constexpr (NodeTraits::propagate_on_container_move_assignment::value) {
            if (other.alloc_ != alloc_) {
              alloc_ = std::move(other.alloc_);
            }
          }
          swap_nodes(other);
          return *this;
        }

        ~ForwardList() noexcept(noexcept(delete_all())) { delete_all(); }

        template <typename... Args>
        void emplace_back(Args&&... args) noexcept(noexcept(
                create_node(std::declval<BasicNode*&>(), std::declval<size_t&>(),
                            std::forward<Args>(args)...))) {
          Node* new_node = create_node(&root, 0, std::forward<Args>(args)...);
          root.prev->next = new_node;
          root.prev = new_node;
          ++sz;
        }

        void attach_after(BasicNode* where, Node* node) noexcept {
          BasicNode* tmp = where->next;
          where->next = node;
          node->next = tmp;
          if (root.prev == where) {
            root.prev = node;
          }
          ++sz;
        }

        void pop_front() noexcept(noexcept(NodeTraits::deallocate(
                std::declval<NodeAllocator&>(), std::declval<Node*&>(),
                std::declval<
                        size_t&>())) && noexcept(NodeTraits::
        destroy(std::declval<NodeAllocator&>(),
                std::declval<Node*&>()))) {
          BasicNode* second_node = root.next->next;
          NodeTraits::destroy(alloc_, static_cast<Node*>(root.next));
          NodeTraits::deallocate(alloc_, static_cast<Node*>(root.next), 1);
          root.next = second_node;
          --sz;
          if (sz == 0) {
            root.prev = &root;
          }
        }

        void erase_after(BasicNode* node) noexcept(noexcept(NodeTraits::deallocate(
                std::declval<NodeAllocator&>(), std::declval<Node*&>(),
                std::declval<
                        size_t&>())) && noexcept(NodeTraits::
        destroy(std::declval<NodeAllocator&>(),
                std::declval<Node*&>()))) {
          if (node->next->next == &root) {
            root.prev = node;
          }
          BasicNode* nxt = node->next->next;
          NodeTraits::destroy(alloc_, static_cast<Node*>(node->next));
          NodeTraits::deallocate(alloc_, static_cast<Node*>(node->next), 1);
          node->next = nxt;
          --sz;
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////   HASHTABLE   //////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    using TableAllocator =
            typename AllocTraits::template rebind_alloc<BasicNode*>;
    using TableTraits = std::allocator_traits<TableAllocator>;

    struct HashTable {
        [[no_unique_address]] TableAllocator table_alloc;
        size_t occupied = 0;
        size_t cap = 0;
        BasicNode** table_arr = nullptr;

        void clear(BasicNode* node) noexcept {
          for (size_t ind = 0; ind < cap; ++ind) {
            table_arr[ind] = node;
          }
        }

        HashTable(size_t cap) noexcept(noexcept(TableTraits::allocate(
                std::declval<TableAllocator&>(), std::declval<size_t&>())))
                : cap(cap), table_arr(TableTraits::allocate(table_alloc, cap)) {
          clear(nullptr);
        }

        HashTable() noexcept(noexcept(HashTable(std::declval<size_t>())))
                : HashTable(1ULL) {}

        HashTable(size_t cap, const TableAllocator& alloc) noexcept(
        noexcept(TableTraits::allocate(std::declval<TableAllocator&>(),
                                       std::declval<size_t&>())))
                : table_alloc(alloc),
                  cap(cap),
                  table_arr(TableTraits::allocate(table_alloc, cap)) {
          clear(nullptr);
        }

        HashTable(const TableAllocator& alloc) noexcept(noexcept(
                HashTable(std::declval<size_t>(), std::declval<TableAllocator&>())))
                : HashTable(1, alloc) {}

        HashTable(const HashTable& other) noexcept(noexcept(TableTraits::allocate(
                std::declval<TableAllocator&>(), std::declval<size_t&>())))
                : table_alloc(TableTraits::select_on_container_copy_construction(
                other.table_alloc)),
                  occupied(other.occupied),
                  cap(other.cap),
                  table_arr(TableTraits::allocate(table_alloc, cap)) {
          memcpy(table_arr, other.table_arr, cap * sizeof(Node*));
        }

        ~HashTable() noexcept(noexcept(TableTraits::deallocate(
                std::declval<TableAllocator&>(), std::declval<BasicNode**&>(),
                std::declval<size_t&>()))) {
          if (table_arr != nullptr) {
            TableTraits::deallocate(table_alloc, table_arr, cap);
          }
        }

        HashTable(HashTable&& other) noexcept
                : table_alloc(std::move(other.table_alloc)) {
          std::swap(occupied, other.occupied);
          std::swap(cap, other.cap);
          std::swap(table_arr, other.table_arr);
        }

        void swap(HashTable& other) noexcept {
          if constexpr (TableTraits::propagate_on_container_swap::value) {
            if (table_alloc != other.table_alloc) {
              table_alloc.swap(other.alloc_);
            }
          } else {
            assert(table_alloc == other.table_alloc);
            // UB if propagate_on_container_swap is false and allocators are not
            // equal https://en.cppreference.com/w/cpp/named_req/Allocator
          }
          std::swap(cap, other.cap);
          std::swap(occupied, other.occupied);
          std::swap(table_arr, other.table_arr);
        }

        HashTable&
        operator=(const HashTable& other) noexcept(noexcept(TableTraits::allocate(
                std::declval<TableAllocator&>(),
                std::declval<
                        size_t&>())) && noexcept(TableTraits::
        deallocate(
                std::declval<TableAllocator&>(),
                std::declval<BasicNode**&>(),
                std::declval<size_t&>()))) {
          BasicNode** new_arr;
          TableAllocator new_alloc = table_alloc;
          if constexpr (TableTraits::propagate_on_container_copy_assignment::
          value) {
            new_alloc = other.table_alloc;
          }
          new_arr = TableTraits::allocate(new_alloc, other.cap);
          memcpy(new_arr, other.table_arr, other.cap * sizeof(BasicNode*));
          try {
            TableTraits::deallocate(table_alloc, table_arr, cap);
          } catch (...) {
            TableTraits::deallocate(new_alloc, other.cap);
          }
          table_alloc = new_alloc;
          table_arr = new_arr;
          occupied = other.occupied;
          cap = other.cap;
          return *this;
        }

        HashTable&
        operator=(HashTable&& other) noexcept(noexcept(TableTraits::allocate(
                std::declval<TableAllocator&>(),
                std::declval<
                        size_t&>())) && noexcept(TableTraits::
        deallocate(
                std::declval<TableAllocator&>(),
                std::declval<BasicNode**&>(),
                std::declval<size_t&>()))) {
          BasicNode* new_arr;
          if (other.table_alloc != table_alloc) {
            if constexpr (!(TableTraits::propagate_on_container_move_assignment::
            value)) {
              new_arr = TableTraits::allocate(table_alloc, other.cap);
              memcpy(new_arr, other.table_arr, cap * sizeof(BasicNode*));
              try {
                TableTraits::deallocate(table_alloc, table_arr, cap);
              } catch (...) {
                TableTraits::deallocate(table_alloc, new_arr, other.cap);
              }
            } else {
              TableTraits::deallocate(table_alloc, table_arr, cap);
            }
            table_arr = nullptr;
            cap = 0;
            occupied = 0;
          }
          if constexpr (TableTraits::propagate_on_container_move_assignment::
          value) {
            table_alloc = std::move(other.table_alloc);
          }
          if (other.table_alloc == table_alloc ||
              TableTraits::propagate_on_container_move_assignment::value) {
            std::swap(occupied, other.occupied);
            std::swap(cap, other.cap);
            std::swap(table_arr, other.table_arr);
            return *this;
          }
          cap = other.cap;
          occupied = other.occupied;
          table_arr = new_arr;
          return *this;
        }

        [[nodiscard]] float get_load_factor() noexcept {
          if (cap == 0) {
            return 1.0f;
          }
          return static_cast<float>(occupied) / static_cast<float>(cap);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    /////////////////////////////   FIELDS   //////////////////////////////
    ///////////////////////////////////////////////////////////////////////

    [[no_unique_address]] Hash hash_;
    [[no_unique_address]] Equal eq_;

    float max_load_f_ = 0.8f;
    HashTable table_;
    ForwardList list_;

    ///////////////////////////////////////////////////////////////////////
    /////////////////////////////   METHODS   /////////////////////////////
    ///////////////////////////////////////////////////////////////////////

    BasicNode* find_prev(const HashTable& table, const ForwardList& list,
                         const Key& key) const
    noexcept(noexcept(hash_(key)) && noexcept(
            eq_(key, std::declval<const Key&>()))) {
      BasicNode* root_ptr = const_cast<BidirectNode*>(&(list.root));
      BasicNode* found = root_ptr;
      if (table.table_arr == nullptr) {
        return found;
      }
      size_t curr_hash = hash_(key) % table.cap;
      BasicNode* node_found = table.table_arr[curr_hash];
      found = (node_found == nullptr) ? root_ptr : node_found;
      while (found->next != root_ptr) {
        if (static_cast<Node*>(found->next)->hash_ind != curr_hash) {
          break;
        }
        if (eq_(key, static_cast<Node*>(found->next)->keyval.first)) {
          return found;
        }
        found = found->next;
      }
      found = root_ptr;
      return found;
    }

    void insert_node(HashTable& table, ForwardList& list, Node* node) noexcept(
    noexcept(hash_(std::declval<const Key&>())) && noexcept(
            list.attach_after(std::declval<BasicNode*>(), node))) {
      size_t curr_ind = hash_(node->keyval.first) % table.cap;
      node->hash_ind = curr_ind;
      BasicNode* curr_node = table.table_arr[curr_ind];
      if (curr_node == nullptr) {
        curr_node = &(list.root);
      }
      list.attach_after(curr_node, node);
      if (curr_node == &(list.root) && list.sz >= 2) {
        table.table_arr[static_cast<Node*>(node->next)->hash_ind] = node;
      }
      ++table.occupied;
    }

    template <bool is_const>
    conditional_iterator<is_const> template_find(const Key& key) const
    noexcept(noexcept(find_prev(table_, list_, key)) && noexcept(
            eq_(key, std::declval<const Key&>()))) {
      BasicNode* found = find_prev(table_, list_, key);
      conditional_iterator<is_const> res_it;
      if (found->next == &(list_.root) ||
          !eq_(key, static_cast<Node*>(found->next)->keyval.first)) {
        res_it = conditional_iterator<is_const>(
                const_cast<BidirectNode*>(&(list_.root)));
        return res_it;
      }
      res_it = conditional_iterator<is_const>(found->next);
      return res_it;
    }

    std::pair<HashTable, ForwardList> rehash_to_new(size_t new_cap) noexcept(
    noexcept(HashTable(new_cap, table_.table_alloc)) && noexcept(ForwardList(
            list_.alloc_)) && noexcept(insert_node(std::declval<HashTable&>(),
                                                   std::declval<ForwardList&>(),
                                                   std::declval<Node*>()))) {
      HashTable table(new_cap, table_.table_alloc);
      ForwardList list(list_.alloc_);
      iterator it = begin();
      iterator end_it = end();
      Node* last;
      while (it != end_it) {
        last = static_cast<Node*>(it.node_);
        ++it;
        insert_node(table, list, last);
      }
      list_.detach_nodes();
      return std::make_pair(std::move(table), std::move(list));
    }

public:
    void rehash(size_t new_cap) noexcept(
    noexcept(rehash_to_new(new_cap)) && noexcept(
            table_.swap(std::declval<
                    HashTable&>())) && noexcept(list_
            .swap(std::declval<
                    ForwardList&>()))) {
      std::pair<HashTable, ForwardList> new_hash_list =
              std::move(rehash_to_new(new_cap));
      table_.swap(new_hash_list.first);
      list_.swap_nodes(new_hash_list.second);
    }

    void reserve(size_t count) noexcept(
    noexcept(rehash(std::declval<double>()))) {
      rehash(static_cast<size_t>(
                     std::ceil(static_cast<float>(count) / max_load_f_)));
    }

private:
    void update_table() noexcept(noexcept(reserve(std::declval<size_t>()))) {
      if (table_.get_load_factor() > max_load_f_) {
        reserve(list_.sz > 0 ? list_.sz * 2 : 4);
      }
    }

public:
    [[nodiscard]] float load_factor() const noexcept {
      return table_.get_load_factor();
    }

    [[nodiscard]] float max_load_factor() const noexcept { return max_load_f_; }

    void max_load_factor(float new_max_load_factor) noexcept(
    noexcept(update_table())) {
      max_load_f_ = new_max_load_factor;
      update_table();
    }

    [[nodiscard]] size_t size() const noexcept { return list_.sz; }

    iterator begin() noexcept { return list_.begin(); }

    iterator end() noexcept { return list_.end(); }

    const_iterator begin() const noexcept { return list_.cbegin(); }

    const_iterator end() const noexcept { return list_.cend(); }

    const_iterator cbegin() const noexcept { return list_.cbegin(); }

    const_iterator cend() const noexcept { return list_.cend(); }

    iterator find(const Key& key) noexcept(noexcept(template_find<false>(key))) {
      return template_find<false>(key);
    }

    const_iterator find(const Key& key) const
    noexcept(noexcept(template_find<true>(key))) {
      return template_find<true>(key);
    }

    std::pair<iterator, bool> insert(const NodeType& keyval) noexcept(
    noexcept(find(keyval.first)) && noexcept(list_.create_node(
            &(list_.root), 0,
            keyval)) && noexcept(insert_node(table_, list_,
                                             std::declval<
                                                     Node*>())) && noexcept(update_table())) {
      std::pair<iterator, bool> result = {end(), false};
      iterator found = find(keyval.first);
      if (found != end()) {
        result.first = found;
        return result;
      }
      BasicNode* new_node;
      new_node = list_.create_node(&(list_.root), 0, keyval);
      try {
        insert_node(table_, list_, static_cast<Node*>(new_node));
      } catch (...) {
        list_.delete_node(static_cast<Node*>(new_node));
        throw;
      }
      update_table();
      result.second = true;
      result.first = iterator(new_node);
      return result;
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) noexcept(
    noexcept(find(std::declval<const Key&>())) && noexcept(list_.create_node(
            &(list_.root), 0,
            std::forward<Args>(
                    args)...)) && noexcept(insert_node(table_, list_,
                                                       std::declval<
                                                               Node*>())) && noexcept(update_table())) {
      Node* new_node =
              list_.create_node(&(list_.root), 0, std::forward<Args>(args)...);
      std::pair<iterator, bool> result = {end(), false};
      iterator found;
      try {
        found = find(new_node->keyval.first);
      } catch (...) {
        list_.delete_node(new_node);
      }
      if (found != end()) {
        result.first = found;
        list_.delete_node(new_node);
        return result;
      }
      try {
        insert_node(table_, list_, new_node);
      } catch (...) {
        list_.delete_node(new_node);
        throw;
      }
      update_table();
      result.second = true;
      result.first = iterator(new_node);
      return result;
    }

    template <typename P>
    std::pair<iterator, bool> insert(P&& source) noexcept(
    noexcept(emplace(std::forward<P>(source)))) {
      return emplace(std::forward<P>(source));
    }

    template <typename InputIterator>
    void insert(InputIterator begin, InputIterator end) noexcept(
    noexcept(insert(std::declval<NodeType&>()))) {
      static_assert(
              std::is_base_of_v<
                      std::input_iterator_tag,
                      typename std::iterator_traits<InputIterator>::iterator_category>);
      while (begin != end) {
        insert(*begin);
        ++begin;
      }
    }

private:
    template <bool is_move>
    Value& template_bracket_operator(
            std::conditional_t<is_move, Key&&, const Key&>
            key) noexcept(noexcept(emplace(key, std::move(Value()))
            .first->second) && noexcept(find(key))) {
      iterator found = find(key);
      if (found != end()) {
        return found->second;
      }
      if constexpr (is_move) {
        return emplace(std::move(key), std::move(Value())).first->second;
      } else {
        return emplace(key, std::move(Value())).first->second;
      }
    }

public:
    Value& operator[](const Key& key) noexcept(
    noexcept(template_bracket_operator<false>(key))) {
      return template_bracket_operator<false>(key);
    }

    Value& operator[](Key&& key) noexcept(
    noexcept(template_bracket_operator<true>(std::move(key)))) {
      return template_bracket_operator<true>(std::move(key));
    }

    Value& at(const Key& key) {
      iterator found = find(key);
      if (found != end()) {
        return found->second;
      }
      throw std::out_of_range("UnorderedMap: at: out_of_range");
    }

    void erase(iterator it) noexcept(
    noexcept(list_.erase_after(std::declval<BasicNode*>()))) {
      size_t curr_ind = static_cast<Node*>(it.node_)->hash_ind;
      BasicNode* prev_node = table_.table_arr[curr_ind];
      if (prev_node == nullptr) {
        prev_node = &(list_.root);
      }
      while (prev_node->next != it.node_) {
        prev_node = prev_node->next;
      }
      bool is_single = true;
      if (it.node_->next != &(list_.root)) {
        if (static_cast<Node*>(it.node_->next)->hash_ind != curr_ind) {
          table_.table_arr[static_cast<Node*>(it.node_->next)->hash_ind] =
                  prev_node;
        } else {
          is_single = false;
        }
      }
      if (prev_node != &(list_.root) &&
          static_cast<Node*>(prev_node)->hash_ind == curr_ind) {
        is_single = false;
      }
      if (is_single) {
        table_.table_arr[curr_ind] = nullptr;
      }
      list_.erase_after(prev_node);
    }

    void erase(iterator begin,
               iterator end) noexcept(noexcept(erase(std::declval<iterator>()))) {
      iterator temp;
      while (begin != end) {
        temp = begin;
        ++begin;
        erase(temp);
      }
    }

    UnorderedMap() = default;

    ~UnorderedMap() = default;

    explicit UnorderedMap(size_t bucket_cnt, const Hash& hash = Hash(), const Equal& equal = Equal(), const Allocator& alloc = Allocator()) noexcept(
    noexcept(Hash(hash)) && noexcept(Equal(equal) && noexcept(
            HashTable(bucket_cnt, alloc)) && noexcept(ForwardList(alloc))))
            : hash_(hash), eq_(equal), table_(bucket_cnt, alloc), list_(alloc) {}

    explicit UnorderedMap(const Allocator& alloc) noexcept(
    noexcept(UnorderedMap(0, Hash(), Equal(), alloc)))
            : UnorderedMap(0, Hash(), Equal(), alloc) {}

    explicit UnorderedMap(size_t bucket_cnt, const Allocator& alloc) noexcept(
    noexcept(UnorderedMap(bucket_cnt, Hash(), Equal(), alloc)))
            : UnorderedMap(bucket_cnt, Hash(), Equal(), alloc) {}

    void swap(UnorderedMap& other) noexcept(noexcept(
                                                    std::swap(hash_, other.hash_)) && noexcept(std::swap(eq_, other.eq_))) {
      std::swap(hash_, other.hash_);
      std::swap(eq_, other.eq_);
      table_.swap(other.table_);
      list_.swap(other.list_);
      std::swap(max_load_f_, other.max_load_f_);
    }

    UnorderedMap(const UnorderedMap& other) noexcept(
    noexcept(Hash(other.hash_)) && noexcept(Equal(other.eq_)) && noexcept(
            HashTable(other.table_)) && noexcept(ForwardList(other.list_)))
            : hash_(other.hash_),
              eq_(other.eq_),
              max_load_f_(other.max_load_f_),
              table_(other.table_),
              list_(other.list_) {
      rehash(table_.cap);
    }

    UnorderedMap(UnorderedMap&& other) noexcept(noexcept(
                                                        Hash(std::move(other.hash_))) && noexcept(Equal(std::move(other.eq_))))
            : hash_(std::move(other.hash_)),
              eq_(std::move(other.eq_)),
              max_load_f_(other.max_load_f_),
              table_(std::move(other.table_)),
              list_(std::move(other.list_)) {}

    UnorderedMap& operator=(const UnorderedMap& other) noexcept(noexcept(
                                                                        swap(std::declval<UnorderedMap&>())) && noexcept(UnorderedMap(other))) {
      UnorderedMap new_map(other);
      swap(new_map);
      return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other) noexcept(noexcept(UnorderedMap(
            std::move(other))) && noexcept(swap(std::declval<UnorderedMap&>()))) {
      UnorderedMap new_map(std::move(other));
      swap(new_map);
      return *this;
    }
};
