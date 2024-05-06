#include <iostream>
#include <cstring>
#include <memory>
#include <type_traits>

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

//////////////////////////////////////////////////////////////////////
/////////////////////////////   BLOCKS  //////////////////////////////
//////////////////////////////////////////////////////////////////////

namespace SmartPointersInternals {
  struct BaseControlBlock {
    size_t shared_cnt = 0;
    size_t weak_cnt = 0;

    BaseControlBlock() = default;
    BaseControlBlock(size_t shared, size_t weak) : shared_cnt(shared), weak_cnt(weak) {}
    ~BaseControlBlock() = default;

    [[nodiscard]] virtual void* getPointer() const = 0;

    virtual void deallocateThis() const = 0;

    virtual void destroyObj() = 0;
  };

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  struct ControlBlock : BaseControlBlock {
    U* ptr;

    [[no_unique_address]] Deleter del;
    [[no_unique_address]] Alloc alloc;

    explicit ControlBlock(size_t shared = 0, size_t weak = 0, U* ptr = nullptr, Deleter del = Deleter(), Alloc alloc = Alloc()) : BaseControlBlock(shared, weak), ptr(ptr), del(del), alloc(alloc) {}

    [[nodiscard]] void* getPointer() const override {
      return ptr;
    }

    void destroyObj() override {
      del(ptr);
    }

    void deallocateThis() const override {
      using CBAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlock<U, Deleter, Alloc>>;
      CBAlloc temp_alloc(alloc);
      del.~Deleter();
      alloc.~Alloc();
      std::allocator_traits<CBAlloc>::deallocate(temp_alloc, const_cast<ControlBlock*>(this), 1);
    }
  };

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  struct ContainerBlock : ControlBlock<U, Deleter, Alloc> {
    U object;

    using CB = ControlBlock<U, Deleter, Alloc>;
    using ContainerAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ContainerBlock<U, Deleter, Alloc>>;

    template <typename... Args>
    explicit ContainerBlock(Args&&... args) : object(std::forward<Args>(args)...) {
      CB::ptr = &object;
    }

    void destroyObj() override {
      ContainerAlloc temp_alloc(CB::alloc);
      std::allocator_traits<ContainerAlloc>::destroy(temp_alloc, &object);
    }

    void deallocateThis() const override {
      ContainerAlloc temp_alloc(CB::alloc);
      CB::del.~Deleter();
      CB::alloc.~Alloc();
      std::allocator_traits<ContainerAlloc>::deallocate(temp_alloc, const_cast<ContainerBlock*>(this), 1);
    }
  };
}

//////////////////////////////////////////////////////////////////////
/////////////////////////   SMART POINTERS   /////////////////////////
//////////////////////////////////////////////////////////////////////


template<typename T>
class SharedPtr {

  //////////////////  BLOCKS  //////////////////

  using BaseControlBlock = SmartPointersInternals::BaseControlBlock;

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  using ControlBlock = SmartPointersInternals::ControlBlock<U, Deleter, Alloc>;

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  using ContainerBlock = SmartPointersInternals::ContainerBlock<U, Deleter, Alloc>;

  //////////////////  FIELDS  //////////////////

  T* ptr_obj_ = nullptr;
  BaseControlBlock* block_ = nullptr;

  //////////////////  FRIENDS  //////////////////

  template<typename U, typename... Args>
  friend SharedPtr<U> makeShared(Args&&...);

  template<typename U, typename Alloc, typename... Args>
  friend SharedPtr<U> allocateShared(const Alloc&, Args&&...);

  template <typename U>
  friend class EnableSharedFromThis;

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  //////////////////  METHODS  //////////////////

  explicit SharedPtr(BaseControlBlock* pcb) : ptr_obj_(static_cast<T*>(pcb->getPointer())), block_(pcb) {
    ++pcb->shared_cnt;
  }

  void release() {
    if (block_ != nullptr) {
      --block_->shared_cnt;

      if (block_->shared_cnt == 0) {
        block_->destroyObj();

        if (block_->weak_cnt == 0) {
          block_->deallocateThis();
        }
      }

      ptr_obj_ = nullptr;
      block_ = nullptr;
    }
  }

public:

  SharedPtr() = default;

  SharedPtr(const SharedPtr<T>& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->shared_cnt;
    }
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->shared_cnt;
    }
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    other.ptr_obj_ = nullptr;
    other.block_ = nullptr;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    release();
    ptr_obj_ = static_cast<T*>(other.ptr_obj_);
    block_ = other.block_;

    if (block_ != nullptr) {
      ++block_->shared_cnt;
    }

    return *this;
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& other) {
    release();
    ptr_obj_ = static_cast<T*>(other.ptr_obj_);
    block_ = other.block_;

    if (block_ != nullptr) {
      ++block_->shared_cnt;
    }

    return *this;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& other) {
    release();
    std::swap(ptr_obj_, reinterpret_cast<T*&>(other.ptr_obj_));
    std::swap(block_, other.block_);
    return *this;
  }

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  SharedPtr(U* ptr, Deleter del = std::default_delete<U>(), Alloc alloc = Alloc()) : ptr_obj_(static_cast<T*>(ptr)) {
    using CBAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlock<U, Deleter, Alloc>>;

    CBAlloc temp_alloc = alloc;
    ControlBlock<U, Deleter, Alloc>* new_block = std::allocator_traits<CBAlloc>::allocate(temp_alloc, 1);
    ::new (new_block) ControlBlock<U, Deleter, Alloc>(1, 0, ptr, del, alloc);

    block_ = static_cast<BaseControlBlock*>(new_block);

    if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
      ptr->shared_block_ptr_ = block_;
    }
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& other, T* ptr) : ptr_obj_(ptr), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->shared_cnt;
    }
  }

  ~SharedPtr() {
    release();
  }

  [[nodiscard]] size_t use_count() const {
    if (ptr_obj_ == nullptr) {
      return 0ULL;
    }

    return block_->shared_cnt;
  }

  template <typename U = T>
  void reset(U* ptr = nullptr) {
    release();

    if (ptr == nullptr) {
      return;
    }

    SharedPtr new_smart_ptr(ptr);
    *this = std::move(new_smart_ptr);
  }

  T* operator->() noexcept {
    return ptr_obj_;
  }

  const T* operator->() const noexcept {
    return ptr_obj_;
  }

  T& operator*() noexcept {
    return *ptr_obj_;
  }

  const T& operator*() const noexcept {
    return *ptr_obj_;
  }

  T* get() noexcept {
    return ptr_obj_;
  }

  const T* get() const noexcept {
    return ptr_obj_;
  }

  template <typename U>
  void swap(SharedPtr<U>& other) noexcept {
    std::swap(ptr_obj_, other.ptr_obj_);
    std::swap(block_, other.block_);
  }
};

template <typename T>
class WeakPtr {

  //////////////////  BLOCKS  //////////////////

  using BaseControlBlock = SmartPointersInternals::BaseControlBlock;

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  using ControlBlock = SmartPointersInternals::ControlBlock<U, Deleter, Alloc>;

  //////////////////  FIELDS  //////////////////

  T* ptr_obj_ = nullptr;
  BaseControlBlock* block_ = nullptr;

  //////////////////  FRIENDS  //////////////////

  template <typename U>
  friend class EnableSharedFromThis;

  template <typename U>
  friend class WeakPtr;

  template <typename U>
  friend class SharedPtr;

  //////////////////  METHODS  //////////////////

  explicit WeakPtr(BaseControlBlock* pcb) : ptr_obj_(static_cast<T*>(pcb->getPointer())), block_(pcb) {
    ++pcb->weak_cnt;
  }

  void release() {
    if (block_ != nullptr) {
      --block_->weak_cnt;

      if (block_->shared_cnt == 0 && block_->weak_cnt == 0) {
        block_->deallocateThis();
      }

      ptr_obj_ = nullptr;
      block_ = nullptr;
    }
  }

public:

  WeakPtr() = default;

  template <typename U>
  WeakPtr(const WeakPtr<U>& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->weak_cnt;
    }
  }

  template <typename U>
  WeakPtr(const SharedPtr<U>& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->weak_cnt;
    }
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) : ptr_obj_(static_cast<T*>(other.ptr_obj_)), block_(other.block_) {
    other.ptr_obj_ = nullptr;
    other.block_ = nullptr;
  }

  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& other) {
    release();
    ptr_obj_ = static_cast<T*>(other.ptr_obj_);
    block_ = other.block_;

    if (block_ != nullptr) {
      ++block_->weak_cnt;
    }

    return *this;
  }

  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& other) {
    release();
    ptr_obj_ = static_cast<T*>(other.ptr_obj_);
    block_ = other.block_;

    if (block_ != nullptr) {
      ++block_->weak_cnt;
    }

    return *this;
  }

  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& other) {
    release();
    std::swap(ptr_obj_, reinterpret_cast<T*&>(other.ptr_obj_));
    std::swap(block_, other.block_);
    return *this;
  }

  template <typename U, typename Deleter = std::default_delete<U>, typename Alloc = std::allocator<U>>
  WeakPtr(U* ptr, Deleter del = std::default_delete<U>(), Alloc alloc = Alloc()) : ptr_obj_(static_cast<T*>(ptr)) {
    using CBAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ControlBlock<U, Deleter, Alloc>>;

    CBAlloc temp_alloc = alloc;
    ControlBlock<U, Deleter, Alloc>* new_block = std::allocator_traits<CBAlloc>::allocate(temp_alloc, 1);
    ::new (new_block) ControlBlock<U, Deleter, Alloc>(0, 1, ptr, del, alloc);

    block_ = static_cast<BaseControlBlock*>(new_block);

    if constexpr (std::is_base_of_v<EnableSharedFromThis<U>, U>) {
      ptr->shared_block_ptr_ = block_;
    }
  }

  template <typename U>
  WeakPtr(const WeakPtr<U>& other, T* ptr) : ptr_obj_(ptr), block_(other.block_) {
    if (block_ != nullptr) {
      ++block_->weak_cnt;
    }
  }

  ~WeakPtr() {
    release();
  }

  [[nodiscard]] size_t use_count() const {
    if (ptr_obj_ == nullptr) {
      return 0ULL;
    }

    return block_->shared_cnt;
  }

  T* operator->() noexcept {
    return ptr_obj_;
  }

  const T* operator->() const noexcept {
    return ptr_obj_;
  }

  T& operator*() noexcept {
    return *ptr_obj_;
  }

  const T& operator*() const noexcept {
    return *ptr_obj_;
  }

  T* get() noexcept {
    return ptr_obj_;
  }

  const T* get() const noexcept {
    return ptr_obj_;
  }

  template <typename U>
  void swap(WeakPtr<U>& other) noexcept {
    std::swap(ptr_obj_, other.ptr_obj_);
    std::swap(block_, other.block_);
  }

  SharedPtr<T> lock() const {
    SharedPtr<T> result(block_);
    return result;
  }

  [[nodiscard]] bool expired() const noexcept {
    return block_ == nullptr || block_->shared_cnt == 0;
  }
};

///////////////////////////////////////////////////////////////////////
/////////////////////////   SUPPORT STRUCTS   /////////////////////////
///////////////////////////////////////////////////////////////////////

template <typename T>
class EnableSharedFromThis {
  typename SharedPtr<T>::BaseControlBlock* shared_block_ptr_ = nullptr;

  template <typename U>
  friend class SharedPtr;

public:
  SharedPtr<T> shared_from_this() const {
    SharedPtr<T> result;

    if (shared_block_ptr_ == nullptr) {
      result = std::move(SharedPtr<T>(static_cast<T*>(const_cast<EnableSharedFromThis*>(this))));
      return result;
    }

    result = std::move(SharedPtr<T>(shared_block_ptr_));
    return result;
  }
};

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(const Alloc& alloc, Args&&... args) {
  using ContainerType = typename SharedPtr<T>::template ContainerBlock<T, std::default_delete<T>, Alloc>;
  using ContainerAlloc = typename std::allocator_traits<Alloc>::template rebind_alloc<ContainerType>;

  ContainerAlloc temp_alloc = alloc;
  ContainerType* new_block = std::allocator_traits<ContainerAlloc>::allocate(temp_alloc, 1);
  std::allocator_traits<ContainerAlloc>::construct(temp_alloc, new_block, std::forward<Args>(args)...);

  auto* block = static_cast<typename SharedPtr<T>::BaseControlBlock*>(new_block);
  SharedPtr<T> result(block);
  return result;
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T, std::allocator<T>, Args...>(std::allocator<T>(), std::forward<Args>(args)...);
}
