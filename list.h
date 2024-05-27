//Created by Yury Vazhenin

#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <cassert>
#include <iterator>
#include <type_traits>
#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <random>
#include <cassert>
#include <deque>
#include <iterator>
#include <compare>
#include <array>

template<size_t N>
class StackStorage {
private:
    size_t current;
    alignas(std::max_align_t) char storage[N];
public:
    StackStorage() : current(0), storage() {}

    template<typename T>
    T* find_place(size_t n) {
        size_t alignment = alignof(T);
        current = ((current + alignment - 1) / alignment) * alignment;
        T* ptr = reinterpret_cast<T*>(storage + current);
        current += sizeof(T) * n;
        return ptr;
    }
};

template <typename T, size_t N>
class StackAllocator {
public:
    using value_type = T;

    StackStorage<N>* storage;

    StackAllocator(StackStorage<N>& storage) : storage(&storage) {}

    template<typename U, size_t M>
    StackAllocator(const StackAllocator<U, M>& alloc) : storage(alloc.storage) {}

    T* allocate (size_t count) {
        return storage->template find_place<T>(count);
    }

    void deallocate (T*, size_t) {}

    template<typename U, size_t M>
    StackAllocator& operator=(const StackAllocator<U, M>& alloc) {
        storage = alloc.storage;
        return *this;
    }

    template<typename U>
    struct rebind {
        using other = StackAllocator<U, N>;
    };

    ~StackAllocator() {}
};

template<typename T, typename Alloc = std::allocator<T>>
class List {
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;

        BaseNode() : prev(this), next(this) {}

        BaseNode(BaseNode* prev, BaseNode* next) : prev(prev), next(next) {}
    };

    struct Node : public BaseNode {
        T value;

        Node() = default;

        Node(const Node& other) = default;

        ~Node() = default;

        Node(BaseNode* prev, BaseNode* next) : BaseNode(prev, next), T() {}

        Node(BaseNode* prev, BaseNode* next, const T& value) : BaseNode(prev, next), value(value) {}

        Node(const T& value) : BaseNode(), value(value) {}
    };

    using AllocType = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using AllocTraits = std::allocator_traits<AllocType>;

    BaseNode fake;
    size_t sz;
    [[no_unique_address]] AllocType alloc;

    template<bool IsConst>
    struct base_iterator {
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using value_type = std::remove_reference_t<reference>;
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = ptrdiff_t;

        BaseNode* ptr;

        base_iterator (const base_iterator<false>& other) : ptr(other.ptr) {}

        base_iterator (BaseNode* ptr) : ptr(ptr) {}

        base_iterator& operator=(const base_iterator<false>& other) {
            ptr = other.ptr;
            return *this;
        }

        reference operator*() const {
            return (static_cast<Node*>(ptr))->value;
        }

        pointer operator->() const {
            return &(*this);
        }

        base_iterator& operator++() {
            ptr = (ptr->next);
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator copy = *this;
            ++*this;
            return copy;
        }

        base_iterator& operator--() {
            ptr = (ptr->prev);
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator copy = *this;
            --*this;
            return copy;
        }

        bool operator==(const base_iterator& other) const {
            return ptr == other.ptr;
        }
    };
public:
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List() : sz(0) {}

    List (size_t sz_) : List() {
        sz = sz_;
        fill_list();
    }

    List (size_t sz_, const T& value) : List() {
        sz = sz_;
        fill_list(value);
    }

    List(const Alloc& alloc) : sz(0), alloc(alloc) {}

    List (size_t sz_, const Alloc& allocator) : List(allocator) {
        sz = sz_;
        fill_list();
    }

    List (size_t sz_, const T& value, const Alloc& allocator) : List(allocator){
        sz = sz_;
        fill_list(value);
    }

    bool empty() const {
        return (sz == 0);
    }

    void push_back (const T& value) {
        insert(end(), value);
    }

    void push_front (const T& value) {
        insert(begin(), value);
    }

    void pop_back() {
        pop_node(fake.prev);
    }

    void pop_front() {
        pop_node(fake.next);
    }

    void erase (const_iterator place) {
        pop_node(place.ptr);
    }

    void insert (const_iterator place, const T& value) {
        BaseNode* next_node = place.ptr;
        Node* new_node = add_node(place.ptr->prev, alloc, value);
        new_node->next = next_node;
        next_node->prev = new_node;
        ++sz;
    }

    size_t size() const {
        return sz;
    }

    AllocType get_allocator () const {
        return alloc;
    }

    iterator begin() {
        return iterator(fake.next);
    }

    const_iterator begin() const {
        return cbegin();
    }

    const_iterator cbegin() const {
        return const_iterator(fake.next);
    }

    iterator end() {
        return iterator(&fake);
    }

    const_iterator end() const{
        return cend();
    }

    const_iterator cend() const {
        return const_iterator(const_cast<BaseNode*>(&fake));
    }

    reverse_iterator rbegin() {
        return reverse_iterator(&fake);
    }

    const_reverse_iterator rbegin() const {
        return crbegin();
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(const_cast<BaseNode*>(&fake));
    }

    reverse_iterator rend() {
        return reverse_iterator(&fake);
    }

    const_reverse_iterator rend() const{
        return crend();
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(const_cast<BaseNode*>(&fake));
    }

    List (const List& other) : List(AllocTraits::select_on_container_copy_construction(other.alloc)) {
        for (const auto& elem : other) {
            push_back(elem);
        }
    }


    void swap(List& other) {
        std::swap(fake, other.fake);
        std::swap(sz, other.sz);
        std::swap(alloc, other.alloc);
        if (other.sz != 0) {
            other.fake.next->prev = &other.fake;
            other.fake.prev->next = &other.fake;
        } else {
            other.fake.next = other.fake.prev = &other.fake;
        }
        if (sz != 0) {
            fake.next->prev = &fake;
            fake.prev->next = &fake;
        } else {
            fake.next = fake.prev = &fake;
        }
    }

    List& operator=(const List& other) {
        if (this == &other)
            return *this;
        List new_list(AllocTraits::propagate_on_container_copy_assignment::value ? other.alloc : alloc);
        for (const auto& elem : other) {
            new_list.push_back(elem);
        }
        clear();
        swap(new_list);
        return *this;
    }

    ~List() {
        clear();
    }

private:
    void clear() {
        while (!empty())
            pop_back();
    }

    template<typename... Args>
    Node* add_node(BaseNode* previous, AllocType& alloc_for_add, Args&... args) {
        Node* new_node = AllocTraits::allocate(alloc_for_add, 1);
        try {
            AllocTraits::construct(alloc_for_add, new_node, args...);
        } catch (...) {
            AllocTraits::deallocate(alloc_for_add, new_node, 1);
            throw;
        }
        previous->next = new_node;
        new_node->prev = previous;
        return new_node;
    }

    template<typename... Args>
    void fill_list(Args&... args) {
        BaseNode* previous_node = &fake;
        size_t count = sz;
        sz = 0;
        for (; sz < count; ++sz) {
            Node *current_node = add_node(previous_node, alloc, args...);
            previous_node = current_node;
            fake.prev = current_node;
            current_node->next = &fake;
        }
    }

    void pop_node (BaseNode* ptr) {
        if (ptr == &fake)
            return;
        BaseNode* next_node = ptr->next;
        BaseNode* prev_node = ptr->prev;
        AllocTraits::destroy(alloc, static_cast<Node*>(ptr));
        AllocTraits::deallocate(alloc, static_cast<Node*>(ptr), 1);
        prev_node->next = next_node;
        next_node->prev = prev_node;
        --sz;
    }
};
