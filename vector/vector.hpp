#pragma once

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <type_traits>

namespace np {
    template <typename T, typename Allocator = std::allocator<T>>
    class vector {
    public:

        // Allocator
        using allocator_type = Allocator;
        using allocator_traits = std::allocator_traits<allocator_type>;

        // Type
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = const pointer;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

    private:

        size_type capacity_ = 0;
        size_type size_ = 0;

        pointer data_ = nullptr;

        allocator_type allocator_;

        template <bool is_const>
        class base_iterator {
        public:
            using pointer_type = std::conditional_t<is_const, const_pointer, pointer>;
            using reference_type = std::conditional_t<is_const, const_reference, reference>;
            using difference_type = std::ptrdiff_t;
            using value_type = std::remove_cv_t<std::remove_pointer_t<pointer_type>>;
            using iterator_category = std::random_access_iterator_tag;

            pointer_type ptr_ = nullptr;
            pointer_type begin_ = nullptr;
            pointer_type end_ = nullptr;

            /***************************/
            base_iterator() noexcept = default;

            explicit base_iterator(pointer_type ptr, pointer_type begin = nullptr, pointer_type end = nullptr) noexcept : ptr_(ptr), begin_(begin), end_(end) {}

            base_iterator(const base_iterator& other) noexcept : ptr_(other.ptr_), begin_(other.begin_), end_(other.end_) {}

            base_iterator(base_iterator&& other) noexcept = default; // *

            base_iterator& operator=(const base_iterator& other) noexcept = default;

            base_iterator& operator=(base_iterator&& other) noexcept = default; // *
            /***************************/


            /***************************/
            reference_type operator*() const {
                return *ptr_;
            }

            pointer_type operator->() const {
                return ptr_;
            }
            /***************************/



            /***************************/
            base_iterator& operator++() {
                ++ptr_;
                return *this;
            }

            base_iterator& operator--() {
                --ptr_;
                return *this;
            }

            base_iterator operator++(int) {
                base_iterator temp = *this;
                ++ptr_;
                return temp;
            }

            base_iterator operator--(int) {
                base_iterator temp = *this;
                --ptr_;
                return temp;
            }
            /***************************/



            /***************************/
            base_iterator operator+(difference_type value) const {
                return base_iterator(ptr_ + value, begin_, end_);
            }

            base_iterator operator-(difference_type value) const {
                return base_iterator(ptr_ - value, begin_, end_);
            }

            difference_type operator-(const base_iterator& other) const {
                return ptr_ - other.ptr_;
            }

            base_iterator& operator+=(difference_type value) {
                ptr_ += value;
                return *this;
            }

            base_iterator& operator-=(difference_type value) {
                ptr_ -= value;
                return *this;
            }
            /***************************/



            /***************************/
            bool operator==(const base_iterator& other) const {
                return ptr_ == other.ptr_;
            }

            bool operator!=(const base_iterator& other) const {
                return ptr_ != other.ptr_;
            }

            bool operator<(const base_iterator& other) const {
                return ptr_ < other.ptr_;
            }

            bool operator<=(const base_iterator& other) const {
                return ptr_ <= other.ptr_;
            }

            bool operator>(const base_iterator& other) const {
                return ptr_ > other.ptr_;
            }

            bool operator>=(const base_iterator& other) const {
                return ptr_ >= other.ptr_;
            }
            /***************************/



            /***************************/
            explicit operator base_iterator<true>() const {
                return base_iterator<true>(ptr_, begin_, end_);
            }

            explicit operator base_iterator<false>() const {
                return base_iterator<false>(ptr_, begin_, end_);
            }
            /***************************/
        };
    public:
        using iterator = base_iterator<false>;
        using const_iterator = base_iterator<true>;

        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        vector() = default;

        explicit vector( const allocator_type& alloc = Allocator()) {
            resize(alloc);
        }

        explicit vector(const size_type n) : capacity_(n), size_(n), data_(allocator_traits::allocate(allocator_, n)) {
            std::uninitialized_default_construct_n(data_, n);
        }

        vector(const size_type n, const_reference value) : capacity_(n), size_(n), data_(allocator_traits::allocate(allocator_, n)) {
            std::uninitialized_fill_n(data_, n, value);
        }

        vector(const std::initializer_list<T>& list) : capacity_(list.size()), size_(list.size()), data_(allocator_traits::allocate(allocator_, capacity_)) {
            std::uninitialized_copy(list.begin(), list.end(), data_);
        }

        template <class InputIt>
        vector(InputIt first, InputIt last) {
            if (first == last) {
                return;
            }

            size_type distance = std::distance(first, last);
            reserve(distance);

            for (; first != last; ++first) {
                push_back(*first);
            }
        }

        vector (const vector& other) {
            Allocator new_allocator = allocator_traits::select_on_container_copy_construction()
                ? other.allocator_
                : allocator_;

            const_pointer new_arr = allocator_traits::allocate(new_allocator, other.capacity_);

            size_type index = 0;
            try {
                for (; index < other.size_; ++index) {
                    allocator_traits::construct(new_allocator, new_arr + index, other[index]);
                }
            } catch (...) {
                for (size_type i = 0; i < index; ++i) {
                    allocator_traits::destroy(new_allocator, new_arr + i);
                }

                allocator_traits::deallocate(new_allocator, new_arr, other.capacity_);
                throw;
            }

            if (data_ != nullptr) {
                for (size_type i = 0; i < other.size_; ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }

                allocator_traits::deallocate(allocator_, data_, capacity_);
            }

            data_ = new_arr;
            size_ = other.size_;
            capacity_ = other.capacity_;
            allocator_ = new_allocator;
        }

        vector(vector&& other) noexcept {
            if (this != &other) {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
        }

        vector& operator=(const vector& other) {
            Allocator new_allocator = allocator_traits::propagate_on_container_copy_assignment::value
                ? other.allocator_
                : allocator_;

            pointer new_arr = allocator_traits::allocate(new_allocator, other.capacity_);

            size_type index = 0;
            try {
                for (; index < other.size_; ++index) {
                    allocator_traits::construct(new_allocator, new_arr + index, other[index]);
                }
            } catch (...) {
                for (size_type i = 0; i < index; ++i) {
                    allocator_traits::destroy(new_allocator, new_arr + i);
                }
                allocator_traits::deallocate(new_allocator, new_arr, other.capacity_);
                throw;
            }

            if (data_ != nullptr) {
                for (size_type i = 0; i < size_; ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }

                allocator_traits::deallocate(allocator_, data_, capacity_);
            }

            data_ = new_arr;
            size_ = other.size_;
            capacity_ = other.capacity_;

            return *this;
        }

        vector& operator=(vector&& other) noexcept {
            if (this != &other) {
                if (data_ != nullptr) {
                    for (size_type i = 0; i < size_; ++i) {
                        allocator_traits::destroy(allocator_, data_ + i);
                    }

                    allocator_traits::deallocate(allocator_, data_, capacity_);
                }

                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
                    allocator_ = std::move(other.allocator_);
                }

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }

            return *this;
        }

        vector& operator=( std::initializer_list<value_type> ilist) {
            if (data_ != nullptr) {
                for (size_type i{}; i < ilist.size(); ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }
                allocator_traits::deallocate(allocator_, data_, capacity_);
            }

            data_ = allocator_traits::allocate(allocator_, ilist.size());
            capacity_ = ilist.size();
            size_ = 0;

            try {
                for (const auto& item : ilist) {
                    allocator_traits::construct(allocator_, data_ + size_, item);
                    ++size_;
                }
            } catch (...) {
                // Откат при исключении
                for (size_type i = 0; i < size_; ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }
                allocator_traits::deallocate(allocator_, data_, capacity_);
                throw;
            }

            return *this;
        }

        void reserve(const size_type new_capacity) {
            if (new_capacity <= capacity_) {
                return;
            }

            const_pointer new_arr = allocator_traits::allocate(allocator_, new_capacity);

            size_type index = 0;
            try {
                for (; index < size_; ++index) {
                    allocator_traits::construct(allocator_, new_arr + index, data_[index]);
                }
            }
            catch (...) {
                for (size_type i = 0; i < index; ++i) {
                    allocator_traits::destroy(allocator_, new_arr + i);
                }

                allocator_traits::deallocate(allocator_, new_arr, capacity_);
                throw;
            }

            for (size_type i = 0; i < size_; ++i) {
                allocator_traits::destroy(allocator_, data_ + i);
            }

            allocator_traits::deallocate(allocator_, data_, new_capacity);

            data_ = new_arr;
            capacity_ = new_capacity;
        }

        void push_back(const_reference element) {
            emplace_back(element);
        }

        void push_back(value_type&& element) {
            emplace_back(std::move(element));
        }

        template<typename... Args>
        void emplace_back(Args&&... args) {
            if (size_ == capacity_) {
                reserve(capacity_ ? capacity_ * 2 : 1);
            }
            allocator_traits::construct(allocator_, data_ + size_, std::forward<Args>(args)...);
            ++size_;
        }

        void pop_back() {
            allocator_traits::destroy(allocator_, data_ + --size_);
        }

        void clear() {
            for (; size_ > 0; --size_) {
                allocator_traits::destroy(allocator_, data_ + size_ - 1);
            }
        }

        void shrink_to_fit() {
            if (size_ < capacity_) {
                const_pointer new_arr = allocator_traits::allocate(allocator_, size_);

                size_type index = 0;
                try {
                    for (; index < size_; ++index) {
                        allocator_traits::construct(allocator_, new_arr + index, data_[index]);
                    }
                } catch (...) {
                    for (; index > 0; --index) {
                        allocator_traits::destroy(allocator_, new_arr + index - 1);
                    }

                    allocator_traits::deallocate(allocator_, new_arr, size_);
                    throw;
                }

                for (size_type i = size_; i < capacity_; ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }

                allocator_traits::deallocate(allocator_, data_, capacity_);

                data_ = new_arr;
                capacity_ = size_;
            }
        }

       void resize(const size_type count) {
            if (count > capacity_) {
                reserve(count);
            }

            if (count == 0) {
                clear();
            }
            else {
                if (count > size_) {
                    try {
                        for (size_type i = size_; i < count; ++i) {
                            allocator_traits::construct(allocator_, data_ + i, value_type());
                        }
                    } catch (...) {
                        throw;
                    }
                }
                else {
                    for (size_type i = count; i < size_; ++i) {
                        allocator_traits::destroy(allocator_, data_ + i);
                    }
                }
            }

            size_ = count;
        }

        void resize(const size_type count, const_reference value) {
            if (count > capacity_) {
                reserve(count);
            }

            if (count == 0) {
                clear();
            }
            else {
                if (count > size_) {
                    try {
                        for (size_type i = size_; i < count; ++i) {
                            allocator_traits::construct(allocator_, data_ + i, value);
                        }
                    } catch (...) {
                        throw;
                    }
                }
                else {
                    for (size_type i = count; i < size_; ++i) {
                        allocator_traits::destroy(allocator_, data_ + i);
                    }
                }
            }

            size_ = count;
        }

        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }

        iterator insert(const_iterator pos, const_reference value) {
            difference_type index = pos.ptr_ - data_;
            if (size_ > capacity_ - 1) {
                reserve(capacity_ ? capacity_ * 2 : 1);
            }

            const_pointer ptr = data_ + index;

            if (ptr < data_ + size_) {
                std::uninitialized_copy(ptr, data_ + size_, ptr + 1);
                allocator_traits::destroy(allocator_, ptr);
            }

            allocator_traits::construct(allocator_, ptr, value);
            ++size_;

            return iterator(data_ + index, data_, data_ + size_);
        }

        iterator erase(iterator pos) {
            if (pos.ptr_ < data_ || pos.ptr_ >= data_ + size_) {
                throw std::out_of_range("Iterator out of range");
            }

            pointer ptr = pos.ptr_;

            for (pointer p = ptr + 1; p != data_ + size_; ++p) {
                *(p - 1) = *p;
            }

            allocator_traits::destroy(allocator_, data_ + size_ - 1);

            --size_;

            return iterator(ptr, data_, data_ + size_);
        }

        iterator erase( const_iterator pos ) {
            if (pos.ptr_ < data_ || pos.ptr_ >= data_ + size_) {
                throw std::out_of_range("Iterator out of range");
            }

            const_pointer ptr = pos.ptr_;

            for (pointer p = ptr + 1; p != data_ + size_; ++p) {
                *(p - 1) = *p;
            }

            allocator_traits::destroy(allocator_, data_ + size_ - 1);

            --size_;

            return iterator(ptr, data_, data_ + size_);
        }

        iterator erase(iterator first, iterator last) {
            if (first.ptr_ < data_ || first.ptr_ >= data_ + size_ || last.ptr_ < data_ || last.ptr_ > data_ + size_ || first.ptr_ > last.ptr_) {
                throw std::out_of_range("Iterator out of range");
            }

            pointer ptr_first = first.ptr_;
            pointer ptr_last = last.ptr_;

            for (pointer p = ptr_last; p != data_ + size_; ++p) {
                *(p - std::distance(ptr_first, ptr_last)) = *p;
            }

            for (size_type i = 0; i < std::distance(ptr_first, ptr_last); ++i) {
                allocator_traits::destroy(allocator_, data_ + size_ - i);
            }

            size_ -= std::distance(ptr_first, ptr_last);

            return iterator(ptr_first, data_, data_ + size_);
        }

        iterator erase(const_iterator pos) const noexcept {
            return erase(static_cast<iterator>(pos));
        }

        iterator erase(const_iterator first, const_iterator last) const {
            return erase(static_cast<iterator>(first), static_cast<iterator>(last));
        }

        pointer data() {
            return data_;
        }

        const_pointer data() const {
            return data_;
        }

        size_type max_size() const {
            return std::numeric_limits<std::size_t>::max(); // что это?

        }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

        iterator begin() noexcept { return iterator(data_); }
        const_iterator cbegin() const noexcept { return const_iterator(data_); }

        iterator end() noexcept { return iterator(data_ + size_); }
        const_iterator cend() const noexcept { return const_iterator(data_ + size_); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        reference front() { return *data_; }
        const_reference front() const { return *data_; }

        reference back() { return data_[size_ - 1]; }
        const_reference back() const { return data_[size_ - 1]; }

        reference operator[](const size_type index) {
            return data_[index];
        }

        const_reference operator[](const size_type index) const {
            return data_[index];
        }

        reference at(const size_type index) {
            if (index >= size_) {
                throw std::out_of_range("Index out of range");
            }

            return data_[index];
        }

        const_reference at(const size_type index) const {
            if (index >= size_) {
                throw std::out_of_range("Index out of range");
            }

            return data_[index];
        }

        ~vector() {
            if (data_) {
                for (size_type i = 0; i < size_; ++i) {
                    allocator_traits::destroy(allocator_, data_ + i);
                }

                allocator_traits::deallocate(allocator_, data_, capacity_);
            }
        }
    };
}
