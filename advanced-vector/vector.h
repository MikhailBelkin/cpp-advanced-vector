#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>


template <typename T>
class RawMemory {
public:

    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept {
        buffer_ = std::move(other.buffer_);
        capacity_ = std::move(other.capacity_);
        other.capacity_ = 0;
        other.buffer_ = nullptr;
    }
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        buffer_ = std::move(rhs.buffer_);
        capacity_ = std::move(rhs.capacity_);
        rhs.capacity_ = 0;
        rhs.buffer_ = nullptr;
        return *this;

    }


    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};


template <typename T>
class Vector {
public:


    Vector()
        : capacity_(0)
        , size_(0) {}

    explicit Vector(size_t size)
        : data_(size)
        , capacity_(size)
        , size_(size)  //
    {


        std::uninitialized_value_construct_n(data_.GetAddress(), size);


    }

    Vector(const Vector& other)
        : data_(other.size_)
        , capacity_(other.size_)
        , size_(other.size_)  //
    {

        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());

    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return capacity_;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }


    void Reserve(size_t new_capacity) {

        if (new_capacity <= capacity_) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());

        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);

        capacity_ = new_capacity;
    }

    Vector(Vector&& other) noexcept {

        data_ = std::move(other.data_);
        size_ = std::move(other.size_);
        capacity_ = std::move(other.capacity_);
        other.capacity_ = 0;
        other.size_ = 0;

    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {

                Vector rhs_copy(rhs);
                Swap(rhs_copy);

            }
            else {
                if (rhs.size_ < size_) {
                    for (size_t i = 0; i < rhs.size_; i++) {
                        data_[i] = rhs.data_[i];
                    }
                    DestroyN(data_.GetAddress() + rhs.size_, size_ - rhs.size_);
                    size_ = rhs.size_;
                }
                else {
                    for (size_t i = 0; i < size_; i++) {
                        data_[i] = rhs.data_[i];
                    }
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress() + size_);
                    size_ = rhs.size_;
                }
            }
        }
        return *this;
    }


    Vector& operator=(Vector&& rhs) noexcept {
        {
            if (this != &rhs) {
                if (rhs.size_ > data_.Capacity()) {


                    Swap(rhs);
                    rhs.size_ = 0;
                    rhs.capacity_ = 0;
                }
                else {
                    if (rhs.size_ < size_) {
                        DestroyN(data_.GetAddress(), size_);
                        std::uninitialized_move_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
                        size_ = rhs.size_;
                    }
                    else {
                        std::uninitialized_move_n(rhs.data_.GetAddress() + size_, rhs.size_ - size_, data_.GetAddress());
                        size_ = rhs.size_;
                        capacity_ = std::move(rhs.capacity_);

                    }
                    rhs.size_ = 0;
                    rhs.capacity_ = 0;
                }





            }
            return *this;
        }
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

    void Resize(size_t new_size) {
        if (new_size < size_) {//уменьшаем размер
            DestroyN(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
            return;
        }
        //увеличиваем размер
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
        size_ = new_size;

    }
    void PushBack(const T& value) {
        if (size_ == Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                new (new_data + size_) T(std::move(value));

                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                new (new_data + size_) T(value);
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }

            data_.Swap(new_data);
            capacity_ = new_capacity;
            DestroyN(new_data.GetAddress(), size_);

        }
        else {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                new (data_ + size_) T(std::move(value));

            }
            else {
                new (data_ + size_) T(value);

            }

        }

        ++size_;

    }

    void PushBack(T&& value) {
        if (size_ == Capacity()) {

            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);

            new (new_data + size_) T(std::move(value));

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());

            }
            else {

                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());

            }

            data_.Swap(new_data);
            capacity_ = new_capacity;
            DestroyN(new_data.GetAddress(), size_);

        }
        else {

            new (data_ + size_) T(std::move(value));

        }

        ++size_;

    }
    void PopBack()  noexcept {
        if (size_) {
            Destroy(data_.GetAddress() + size_ - 1);
            size_--;
        }
    }


    template<typename ...Args >
    T& EmplaceBack(Args&&... args) {


        if (size_ == Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                    new (new_data + size_) T(std::forward<Args>(args)...);
                
                        std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());

            }
            else {
                
                    new (new_data + size_) T(std::forward<Args>(args)...);
                
                    std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
                
            }


            data_.Swap(new_data);
            capacity_ = new_capacity;
            DestroyN(new_data.GetAddress(), size_);

        }
        else {
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                new (data_ + size_) T(std::forward<Args>(args)...);

            }
            else {

                new (data_ + size_) T(std::forward<Args>(args)...);

            }

        }

        ++size_;

        return data_[size_ - 1];
    }



    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return data_ + size_;
    }
    const_iterator begin() const noexcept {
        return const_cast<const_iterator>(data_.GetAddress());
    }
    const_iterator end() const noexcept {
        return const_cast<const_iterator>(data_ + size_);
    }
    const_iterator cbegin() const noexcept {
        return const_cast<const_iterator>(data_.GetAddress());
    }
    const_iterator cend() const noexcept {
        return const_cast<const_iterator>(data_ + size_);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        assert(pos >= begin() && pos <= end());

        size_t pos_index = size_==0?0:pos - &data_[0];

        if (size_ == pos_index) {
            EmplaceBack(std::forward<Args>(args)...);
            return &data_[pos_index];

        }


        if (size_ == Capacity()) {
            size_t new_capacity = size_ == 0 ? 1 : size_ * 2;
            RawMemory<T> new_data(new_capacity);



            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                new  (new_data + pos_index) (T)(std::forward<Args>(args)...);

                try {

                    std::uninitialized_move_n(data_.GetAddress(), pos_index, new_data.GetAddress());

                }
                catch (...) {
                    Destroy(data_.GetAddress() + pos_index);
                    throw;
                }
                try {
                    std::uninitialized_move_n(data_.GetAddress() + pos_index, size_ - pos_index, new_data.GetAddress() + pos_index + 1);
                }
                catch (...) {
                    DestroyN(data_.GetAddress(), pos_index);
                    throw;
                }
            }
            else {

                new (new_data + pos_index) T(std::forward<Args>(args)...);

                try {

                    std::uninitialized_copy_n(data_.GetAddress(), pos_index, new_data.GetAddress());

                }
                catch (...) {
                    Destroy(data_.GetAddress() + pos_index);
                    throw;
                }
                try {

                    std::uninitialized_copy_n(data_.GetAddress() + pos_index, size_ - pos_index, new_data.GetAddress() + pos_index + 1);

                }
                catch (...) {
                    DestroyN(data_.GetAddress(), pos_index);
                    throw;
                }
            }

            data_.Swap(new_data);
            capacity_ = new_capacity;
            DestroyN(new_data.GetAddress(), size_);

        }
        else {

            RawMemory<T> temp_data(1);

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                new (temp_data.GetAddress()) T(std::forward<Args>(args)...);

                std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);

            }
            else {

                new (temp_data.GetAddress()) T(std::forward<Args>(args)...);

                std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);

            }

            std::move_backward(data_ + pos_index, data_ + size_-1 , data_ + size_);


            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {

                data_[pos_index] = std::move(temp_data[0]);
                Destroy(temp_data.GetAddress());
            }
            else {

                //data_[pos_index] = temp_data[0];
                data_[pos_index] = std::move(temp_data[0]);
                Destroy(temp_data.GetAddress());
            }

        }

        ++size_;

        return &data_[pos_index];

    }



    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {

        assert(pos >= begin() && pos < end());

        std::move(const_cast<iterator>(pos) + 1, data_ + size_, const_cast<iterator>(pos));
        PopBack();
        if (size_ == 0) return end();
        return const_cast<iterator>(pos);
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);

    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }



    ~Vector() {
        DestroyN(data_.GetAddress(), size_);

    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }


    // Вызывает деструкторы n объектов массива по адресу buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            if (buf) {
                Destroy(buf + i);
            }
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }

    RawMemory<T> data_;
    size_t capacity_ = 0;
    size_t size_ = 0;
};