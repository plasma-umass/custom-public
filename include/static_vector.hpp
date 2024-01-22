// A vector that never copies or moves elements.
// Elements are zero-initilizaed on construction and/or reset.

#ifndef LITTERER_STATIC_VECTOR_HPP
#define LITTERER_STATIC_VECTOR_HPP

#include <cassert>
#include <cstddef>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
template <typename T> class static_vector {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type&;
    using iterator = value_type*;
    using const_iterator = const value_type*;

    static_vector() = default;
    static_vector(const static_vector&) = delete;
    static_vector& operator=(const static_vector&) = delete;
    static_vector(static_vector&&) = default;
    static_vector& operator=(static_vector&&) = default;
    ~static_vector() { delete data_; };

    static_vector(size_type size) : size_(size), data_(new T[size]()) {}

    void reset(size_type size) {
        this->size_ = size;
        delete data_;
        data_ = new T[size]();
    }

    reference operator[](size_type index) { return data_[index]; }

    const_reference operator[](size_type index) const { return data_[index]; }

    reference at(size_type index) {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    const_reference at(size_type index) const {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    reference front() { return data_[0]; }

    const_reference front() const { return data_[0]; }

    reference back() { return data_[size_ - 1]; }

    const_reference back() const { return data_[size_ - 1]; }

    iterator begin() { return data_; }

    const_iterator begin() const noexcept { return data_; }

    const_iterator cbegin() const noexcept { return begin(); }

    iterator end() { return data_ + size_; }

    const_iterator end() const noexcept { return data_ + size_; }

    const_iterator cend() const noexcept { return end(); }

  private:
    size_type size_ = 0;
    value_type* data_ = nullptr;
};
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

#endif
