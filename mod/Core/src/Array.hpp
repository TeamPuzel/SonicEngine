// Created by Lua (TeamPuzel) on May 26th 2025.
// Copyright (c) 2025 All rights reserved.
#pragma once
#include "Primitive.hpp"
#include "Utility.hpp"
#include "Heap.hpp"
#include <type_traits>

namespace core {
    /// A simple array type.
    ///
    /// The empty state of an array is equivalent to zero intialization.
    template <typename T> class Array final {
      public:
        using Index = usize;
        using Element = T;

      private:
        /// Stores the element buffer.
        Element* elements_;                                                   // NOLINT(readability-identifier-naming)
        /// The exact amount of initialized elements.
        usize count_;                                                         // NOLINT(readability-identifier-naming)
        /// The exact amount of elements the buffer can hold.
        usize capacity_;                                                      // NOLINT(readability-identifier-naming)

        /// Append an element without ensuring capacity.
        ///
        /// @param [consume] value The value to forward into the array.
        ///
        /// @pre Capacity must be enough to perform the append and the buffer must be unique.
        /// @post Count is incremented by one and the value placed at the last index.
        template <typename U, std::enable_if_t<std::is_convertible_v<U, Element>, i32> = 0>
        void unsafe_append(U&& value) {
            core::assert(count_ < capacity_, "internal precondition broken, attempted to append beyond capacity");
            new (&elements_[count_]) Element(core::forward<U>(value));
            count_ += 1;
        }

      public:
        /// Ensures the array has enough capacity for a given count of elements.
        ///
        /// @post If capacity is sufficient nothing is done.
        ///       If capacity is insufficient it is increased to exactly the required amount
        ///       by reallocating the buffer and moving elements across.
        void ensure_capacity(usize capacity) {
            if (capacity > capacity_) {
                auto const new_elements = heap::alloc<Element>(capacity);

                for (usize i = 0; i < count_; i += 1)
                    new_elements[i] = core::move(elements_[i]);

                heap::dealloc(elements_);
                elements_ = new_elements;
                capacity_ = capacity;
            }
        }

        /// Creates an empty array. This does not allocate.
        constexpr Array() {
            elements_ = nullptr;
            count_ = 0;
            capacity_ = 0;
        }

        // /// Creates an array from a pack of elements.
        // ///
        // /// @post If the pack is empty the array storage is left empty.
        // ///       If the pack is not empty storage is allocated for the exact number of elements
        // ///       and they are placed at corresponding indices.
        // template <Convertible<Element>... Elements> explicit(false) Array(Elements&&... elements) {
        //     elements_ = heap::alloc<Element>(sizeof...(elements));
        //     count_ = sizeof...(elements);
        //     capacity_ = sizeof...(elements);
        //     (unsafeAppend(core::forward<Elements>(elements)), ...); // SAFETY: We have ensured the capacity
        // }

        // /// @post Destroys all the elements and resets the array to an empty state.
        // auto drop() -> Void {
        //     if (elements_) {
        //         for (UInt i = 0; i < count_; i += 1)
        //             elements_[i].~Element();

        //         heap::dealloc(elements_);
        //         elements_ = nullptr;
        //         count_ = 0;
        //         capacity_ = 0;
        //     }
        // }

        // ~Array() {
        //     drop();
        // }

        // Array(Array const& existing) {
        //     elements_ = heap::alloc<Element>(existing.count_);
        //     count_ = existing.count_;
        //     capacity_ = existing.capacity_;

        //     for (UInt i = 0; i < count_; i += 1)
        //         new (elements_[i]) Element(existing.elements_[i]);
        // }

        // auto operator=(Array const& existing) -> Array& {
        //     // if (this != &existing) {
        //     drop();

        //     elements_ = heap::alloc<Element>(existing.count_);
        //     count_ = existing.count_;
        //     capacity_ = existing.capacity_;

        //     for (UInt i = 0; i < count_; i += 1)
        //         new (&elements_[i]) Element(existing.elements_[i]);
        //     // }
        //     return *this;
        // }

        // Array(Array&& existing) {
        //     elements_ = existing.elements_;
        //     count_ = existing.count_;
        //     capacity_ = existing.capacity_;

        //     existing.elements_ = nullptr;
        //     existing.count_ = 0;
        //     existing.capacity_ = 0;
        // }

        // auto operator=(Array&& existing) -> Array& {
        //     // if (this != &existing) {
        //     drop();

        //     elements_ = existing.elements_;
        //     count_ = existing.count_;
        //     capacity_ = existing.capacity_;

        //     existing.elements_ = nullptr;
        //     existing.count_ = 0;
        //     existing.capacity_ = 0;
        //     // }
        //     return *this;
        // }

        // /// Immutably reference the element at the index. Performs bounds checking in safe builds.
        // ///
        // /// This is not in itself unsafe unless the law of exclusivity was already violated.
        // ///
        // /// @warning Observing this reference beyond just reading it, including escaping it or moving out the element
        // /// is undefined.
        // ///
        // /// @pre The index is within bounds and law of exclusivity is upheld.
        // inline auto operator[](UInt index) const [[clang::lifetimebound]] -> Element const& {
        //     core::precondition(index < count_);
        //     return elements_[index];
        // }

        // /// Immutably reference the element at the index. Performs bounds checking in safe builds.
        // ///
        // /// This is not in itself unsafe unless the law of exclusivity was already violated.
        // ///
        // /// @warning Observing this reference beyond just reading or writing to it,
        // /// including escaping it or moving out the element is undefined. Overlapping mutable access is also
        // undefined.
        // ///
        // /// @pre The index is within bounds and law of exclusivity is upheld.
        // inline auto operator[](UInt index) [[clang::lifetimebound]] -> Element& {
        //     core::precondition(index < count_);
        //     return elements_[index];
        // }

        // /// Get the count of items in this array.
        // ///
        // /// @note This function is pure.
        // [[nodiscard]] [[gnu::pure]] auto count() const -> UInt {
        //     return count_;
        // }

        // /// Check if the array is empty.
        // ///
        // /// @note This function is pure.
        // /// @returns True if the array is empty, false otherwise.
        // [[nodiscard]] [[gnu::pure]] auto isEmpty() const -> Bool {
        //     return count_ == 0;
        // }

        // /// Get the first element of the array.
        // ///
        // /// @note This function is pure.
        // /// @returns The element if the collection is not empty, nil otherwise.
        // [[nodiscard]] [[gnu::pure]] auto first() const -> Optional<Element const&> {
        //     if (isEmpty()) {
        //         return nil;
        //     } else {
        //         return elements_[0];
        //     }
        // }

        // /// Get the last element of the array.
        // ///
        // /// @note This function is pure.
        // /// @returns The element if the array is not empty, nil otherwise.
        // [[nodiscard]] [[gnu::pure]] auto last() const -> Optional<Element const&> {
        //     if (isEmpty()) {
        //         return nil;
        //     } else {
        //         return elements_[count_ - 1];
        //     }
        // }

        // // /// Consume and append an element at the end of the array.
        // // ///
        // // /// @post The storage is grown to accomodate the element,
        // // ///       count is incremented and the element placed at the last index.
        // // template <Convertible<Element> U> auto append(U&& element) -> Void {
        // //     ensureCapacity(count_ + 1);
        // //     unsafeAppend(core::forward<U>(element)); // SAFETY: We have ensured the capacity
        // // }

        // /// Consume and append an element at the end of the array.
        // ///
        // /// @post The storage is grown to accomodate the element,
        // ///       count is incremented and the element placed at the last index.
        // auto append(Element&& element) -> Void {
        //     ensureCapacity(count_ + 1);
        //     unsafeAppend(core::move(element)); // SAFETY: We have ensured the capacity
        // }

        // /// Consume and append an element at the end of the array.
        // ///
        // /// @post The storage is grown to accomodate the element,
        // ///       count is incremented and the element placed at the last index.
        // auto append(Element const& element) -> Void {
        //     ensureCapacity(count_ + 1);
        //     unsafeAppend(element); // SAFETY: We have ensured the capacity
        // }

        // /// Consume and append multiple elements at the end of the array.
        // ///
        // /// @post The storage is grown to accomodate the elements,
        // ///       count is increased and the elements fill the exposed indices.
        // template <Convertible<Element>... Elements> auto append(Elements&&... elements) -> Void {
        //     ensureCapacity(count_ + sizeof...(elements));
        //     (unsafeAppend(core::forward<Elements>(elements)), ...); // SAFETY: We have ensured the capacity
        // }

        // /// Append the contents of another array at the end of this array.
        // ///
        // /// @post The storage is grown to accomodate the elements,
        // ///       count is increased and the elements fill the exposed indices.
        // auto append(Array const& other) -> Void
        //     requires(Copyable<Element>)
        // {
        //     ensureCapacity(count_ + other.count_);
        //     for (auto const& element : other)
        //         unsafeAppend(element); // SAFETY: We have ensured the capacity
        // }

        // [[nodiscard]] auto swapRemove(UInt index) -> Element {
        //     core::precondition(count_ > 0);
        //     core::swap(elements_[index], elements_[count_ - 1]);
        //     auto ret = core::move(elements_[count_ - 1]);
        //     count_ -= 1;
        //     elements_[count_].~Element();
        //     return ret;
        // }

        // auto swapDrop(UInt index) -> Void {
        //     core::precondition(count_ > 0);
        //     core::swap(elements_[index], elements_[count_ - 1]);
        //     count_ -= 1;
        //     elements_[count_].~Element();
        // }

        // /// Destroys the last element and returns a copy.
        // ///
        // /// @pre The array is not empty.
        // [[nodiscard]] auto removeLast() -> Element {
        //     core::precondition(count_ > 0);
        //     auto ret = core::move(elements_[count_ - 1]);
        //     count_ -= 1;
        //     elements_[count_].~Element();
        //     return ret;
        // }

        // /// Destroys the last element.
        // ///
        // /// @pre The array is not empty.
        // /// @post The count is decremented to no longer expose the destroyed value.
        // auto dropLast() -> Void {
        //     core::precondition(count_ > 0);
        //     count_ -= 1;
        //     elements_[count_].~Element();
        // }

        // auto remove(UInt index) -> Element {
        //     core::precondition(index < count_);
        //     auto ret = core::move(elements_[index]);
        //     elements_[index].~Element();
        //     core::mem::copy(&elements_[index + 1], &elements_[index], sizeof(Element) * count_ - index - 1);
        //     count_ -= 1;
        //     return ret;
        // }

        // auto removeFirst() -> Element {
        //     return remove(0);
        // }

        // auto drop(UInt index) -> Void {
        //     remove(index);
        // }
        // auto dropFirst() -> Void {
        //     drop(0);
        // }

        // /// Destroys all the elements in the array.
        // ///
        // /// @post The elements are destroyed and the count set to zero.
        // auto clear() -> Void {
        //     for (UInt i = 0; i < count_; i += 1)
        //         elements_[i].~Element();
        //     count_ = 0;
        // }

        // /// Concatenates two arrays.
        // ///
        // /// @note This function is pure.
        // /// @returns A new array.
        // /// @post The returned array's count is equal to the added counts of self and other.
        // [[nodiscard]] [[gnu::pure]] auto operator+(Array const& other) const -> Array {
        //     Array newArray;
        //     newArray.ensureCapacity(count_ + other.count_);
        //     for (auto const& element : *this)
        //         newArray.unsafeAppend(element);
        //     for (auto const& element : other)
        //         newArray.unsafeAppend(element);
        //     return newArray;
        // }

        // /// Compares two arrays for structural equality.
        // ///
        // /// @note This function is pure.
        // /// @returns Immeadiately returns false if the counts don't compare equal, otherwise compares items one by
        // one.
        // ///          If none compare not equal it finally returns true, otherwise short circuits with false.
        // [[nodiscard]] [[gnu::pure]] auto operator==(Array const& other) const -> Bool
        //     requires(Equatable<Element>)
        // {
        //     if (count() != other.count())
        //         return false;
        //     for (UInt i = 0; i < count(); i += 1)
        //         if (this[i] != other[i])
        //             return false;
        //     return true;
        // }

        // /// Checks if at least one element in this array compares equal to the provided value.
        // ///
        // /// @note This function is pure.
        // [[nodiscard]] [[gnu::pure]] auto contains(Element const& value) const -> Bool
        //     requires(Equatable<Element>)
        // {
        //     for (auto const& element : *this)
        //         if (element == value)
        //             return true;
        //     return false;
        // }

        // /// Get an iterator over this array.
        // [[nodiscard]] auto begin() const [[clang::lifetimebound]] -> Element const* {
        //     return elements_;
        // }
        // /// Get an iterator over this array.
        // [[nodiscard]] auto begin() [[clang::lifetimebound]] -> Element* {
        //     return elements_;
        // }

        // /// Get a past-the-end iterator over this array.
        // [[nodiscard]] auto end() const [[clang::lifetimebound]] -> Element const* {
        //     return elements_ + count_;
        // }
        // /// Get a past-the-end iterator over this array.
        // [[nodiscard]] auto end() [[clang::lifetimebound]] -> Element* {
        //     return elements_ + count_;
        // }

        // [[nodiscard]] auto raw() const [[clang::lifetimebound]] -> Element const* {
        //     return elements_;
        // }
        // [[nodiscard]] auto raw() [[clang::lifetimebound]] -> Element* {
        //     return elements_;
        // }
    };

    // export template <Hashable Element> struct impl::Hashable<Array<Element>> final {
    //     template <Hash H> static auto hash(Array<Element> const& value, Hasher<H>* hasher) -> Void {
    //         for (auto const& element : value)
    //             hasher->combine(element);
    //     }
    // };
}
