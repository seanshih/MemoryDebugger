// an allocator that does not pass through leak detector during allocations

#pragma once
template <class T>
struct free_allocator {
    typedef T value_type;

    free_allocator() {}

    template <class U> 
    free_allocator (const free_allocator<U>&) {}

    T* allocate (std::size_t n) { return static_cast<T*>(std::malloc(n*sizeof(T))); }
    void deallocate (T* p, std::size_t n) { std::free(p); }
    bool operator == (const free_allocator& alloc) const
    {
        // since this allocator is stateless, all alocators are actually the same
        return true;
    }
};

