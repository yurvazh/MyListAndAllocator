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

    template<typename... Args>
    void construct (T* ptr, const Args&... args) {
        new (ptr) T(args...);
    }

    void destroy (T* ptr) {
        ptr->~T();
    }

    StackAllocator select_on_container_copy_construction() const {
        return StackAllocator(*storage);
    }

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
            return &(ptr->value);
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

    List (size_t sz) : sz(sz) {
        fill_default_value();
    }

    List (size_t sz, const T& value) : sz(sz) {
        fill_with_value(value);
    }

    List(const Alloc& alloc) : sz(0), alloc(alloc) {}

    List (size_t sz, const Alloc& allocator) : sz(sz), alloc(allocator) {
        fill_default_value();
    }

    List (size_t sz, const T& value, const Alloc& alloc) : sz(sz), alloc(alloc) {
        fill_with_value(value);
    }

    bool empty() const {
        return (sz == 0);
    }

    void push_back (const T& value) {
        Node* place = add_node_with_value(fake.prev, alloc, value);
        fake.prev = place;
        place->next = &fake;
        ++sz;
    }

    void push_front (const T& value) {
        BaseNode* front = fake.next;
        Node* place = add_node_with_value(&fake, alloc, value);
        place->next = front;
        front->prev = place;
        ++sz;
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
        Node* new_node = add_node_with_value(place.ptr->prev, alloc, value);
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

    List (const List& other) : fake(), sz(other.sz), alloc(AllocTraits::select_on_container_copy_construction(other.alloc)) {
        if (other.sz == 0)
            return;
        const_iterator current_other = other.begin();
        BaseNode* previous_node = &fake;
        size_t index = 0;
        try {
            for (index = 0; index < sz; ++index, ++current_other) {
                Node *current_node = add_node_with_value(previous_node, alloc, *current_other);
                previous_node = current_node;
            }
        } catch (...) {
            clear_prefix(fake, index, alloc);
            throw;
        }
        previous_node->next = &fake;
        fake.prev = previous_node;
    }

    List& operator=(const List& other) {
        size_t new_sz = other.sz;
        BaseNode new_fake = BaseNode();
        AllocType new_alloc = (AllocTraits::propagate_on_container_copy_assignment::value ? other.alloc : alloc);
        const_iterator current_other = other.begin();
        BaseNode* previous_node = &new_fake;
        size_t index = 0;
        try {
            for (index = 0; index < new_sz; ++index, ++current_other) {
                Node *current_node = add_node_with_value(previous_node, new_alloc, *current_other);
                previous_node = current_node;
            }
        } catch (...) {
            clear_prefix(new_fake, index, new_alloc);
            throw;
        }
        previous_node->next = &new_fake;
        new_fake.prev = previous_node;
        clear();
        fake.prev = new_fake.prev;
        fake.next = new_fake.next;
        fake.next->prev = fake.prev->next = &fake;
        alloc = new_alloc;
        sz = new_sz;
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

    Node* add_node_with_value (BaseNode* previous, AllocType& alloc_for_add, const T& value) {
        Node* new_node = AllocTraits::allocate(alloc_for_add, 1);
        try {
            AllocTraits::construct(alloc_for_add, new_node, value);
        } catch (...) {
            AllocTraits::deallocate(alloc_for_add, new_node, 1);
            throw;
        }
        previous->next = new_node;
        new_node->prev = previous;
        return new_node;
    }

    Node* add_default_node (BaseNode* previous, AllocType& alloc_for_add) {
        Node* new_node = AllocTraits::allocate(alloc_for_add, 1);
        try {
            AllocTraits::construct(alloc_for_add, new_node);
        } catch (...) {
            AllocTraits::deallocate(alloc_for_add, new_node, 1);
            throw;
        }
        previous->next = new_node;
        new_node->prev = previous;
        return new_node;
    }

    void clear_prefix (BaseNode& start_node, size_t count, AllocType& alloc_for_clear) {
        if (count == 0) {
            start_node = BaseNode();
            return;
        }
        Node* current_node = static_cast<Node*>(start_node.next);
        Node* next_node = current_node;
        for (size_t index = 0; index < count; ++index) {
            if (index != count - 1) {
                next_node = static_cast<Node*>(current_node->next);
            }
            AllocTraits::destroy(alloc_for_clear, current_node);
            AllocTraits::deallocate(alloc_for_clear, current_node, 1);
            if (index != count - 1) {
                current_node = next_node;
            }
        }
        start_node = BaseNode();
    }

    void fill_with_value(const T& value) {
        BaseNode* previous_node = &fake;
        size_t index = 0;
        try {
            for (index = 0; index < sz; ++index) {
                Node *current_node = add_node_with_value(previous_node, alloc, value);
                previous_node = current_node;
            }
        } catch (...) {
            clear_prefix(fake, index, alloc);
            throw;
        }
        previous_node->next = &fake;
        fake.prev = previous_node;
    }

    void fill_default_value() {
        BaseNode* previous_node = &fake;
        size_t index = 0;
        try {
            for (index = 0; index < sz; ++index) {
                Node *current_node = add_default_node(previous_node, alloc);
                previous_node = current_node;
            }
        } catch (...) {
            clear_prefix(fake, index, alloc);
            throw;
        }
        previous_node->next = &fake;
        fake.prev = previous_node;
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
