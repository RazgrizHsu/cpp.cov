/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Ripcord Software Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
**/

#ifndef MEMSTR_PAGE_PTR_H
#define	MEMSTR_PAGE_PTR_H

#include <atomic>

namespace rz {
namespace memstr {

class Page;

/**
 * A smart pointer impl for StringPages - reasoning:
 * G++ 4.9 doesn't support atomic operations on std::shared_ptr<>, so this
 * pointer implementation takes the strain until 4.9 ages out
**/
class PagePtr final {
public:
    PagePtr() noexcept : ptr_{ nullptr} {}
    PagePtr( std::nullptr_t) noexcept : PagePtr() {}
    explicit PagePtr( Page*) noexcept;
    PagePtr( const PagePtr& rhs) noexcept;
    PagePtr( PagePtr&& rhs) noexcept;
    ~PagePtr() noexcept;

    PagePtr& operator=( const PagePtr& rhs) noexcept;
    PagePtr& operator=( PagePtr&& rhs) noexcept;

    bool operator!() const noexcept { return !get(); }

    Page* operator->() const noexcept { return get(); }
    Page* get() const noexcept { return ptr_.load( std::memory_order_relaxed); }

    std::size_t RefCount() const noexcept;

    bool compare_exchange_strong( PagePtr&, const PagePtr&, std::memory_order) noexcept;
    PagePtr exchange( const PagePtr&, std::memory_order) noexcept;

private:
    friend bool operator==( const PagePtr& lhs, const PagePtr& rhs) noexcept;
    friend bool operator!=( const PagePtr& lhs, const PagePtr& rhs) noexcept;

    std::atomic<Page*> ptr_;
};

inline bool operator==( const PagePtr& lhs, const PagePtr& rhs) noexcept {
    return lhs.get() == rhs.get();
}

inline bool operator!=( const PagePtr& lhs, const PagePtr& rhs) noexcept {
    return lhs.get() != rhs.get();
}

}}

#endif	/* MEMSTR_PAGE_PTR_H */

