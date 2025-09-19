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

#ifndef MEMSTR_PAGE_NURSERY_H
#define	MEMSTR_PAGE_NURSERY_H

#include <atomic>
#include <functional>
#include <cstddef>

#include "page.h"

namespace rz {
namespace memstr {

class PageNursery final {
public:

    using pagesize_t = std::uint32_t;
    using rowcount_t = std::uint32_t;
    using colcount_t = std::uint32_t;

    class Iterator final {
    public:
        Iterator(const Iterator& rhs) noexcept : cols_(rhs.cols_), row_(rhs.row_), start_(rhs.start_), pos_(rhs.pos_) {}
        Iterator& operator=(const Iterator& rhs) noexcept { cols_ = rhs.cols_; row_ = rhs.row_; start_ = rhs.start_; pos_ = rhs.pos_; return *this; }

        bool operator!() const noexcept { return pos_ >= (start_ + cols_); }

    private:
        friend PageNursery;

        Iterator() noexcept : Iterator(0, 0, 0) {}
        Iterator(colcount_t cols, rowcount_t row, colcount_t start) noexcept : cols_(cols), row_(row), start_(start), pos_(start) {}
        rowcount_t operator++(int) noexcept { return pos_++; }

        colcount_t start_;
        colcount_t pos_;
        rowcount_t row_;
        colcount_t cols_;
    };

    using NewPageFunc = std::function<PagePtr( rowcount_t, Page::entrycount_t, Page::entrysize_t)>;

    PageNursery( colcount_t, rowcount_t, pagesize_t, NewPageFunc newPageFunc);
    PageNursery( const PageNursery&) = delete;
    PageNursery( const PageNursery&&) = delete;
    void operator=(const PageNursery&) = delete;

    Iterator Iter(rowcount_t row) const noexcept;
    PagePtr New( rowcount_t);
    PagePtr Next( Iterator& iter);

    inline rowcount_t Rows() const noexcept { return rows_; }
    inline colcount_t Cols() const noexcept { return cols_; }

    std::vector<PagePtr> GetPages( rowcount_t) const;
    std::vector<Page::entrycount_t> GetPageEntryCounts( rowcount_t) const;

private:

    PagePtr Get( colcount_t, rowcount_t);
    PagePtr Get( colcount_t, rowcount_t) const;

    NewPageFunc newPageFunc_;

    std::vector<std::atomic<rowcount_t>> counters_;
    std::vector<PagePtr> data_;

    const colcount_t cols_;
    const rowcount_t rows_;
    const pagesize_t pageSize_;

};

}}

#endif	/* MEMSTR_PAGE_NURSERY_H */

