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

#include "page_nursery.h"
#include "page_sizes.h"

#include <algorithm>

rz::memstr::PageNursery::PageNursery( colcount_t cols, rowcount_t rows, pagesize_t pageSize, NewPageFunc newPageFunc) :
    rows_(rows), cols_(cols), pageSize_(pageSize),
    counters_(rows), data_(rows * cols), newPageFunc_(newPageFunc) {

    std::for_each(counters_.begin(), counters_.end(), [&](decltype(counters_[0])& c) { c.store(0, std::memory_order_relaxed); });
}

rz::memstr::PagePtr rz::memstr::PageNursery::New( rowcount_t row) {
    auto entrySize = PageSizes::GetEntrySize( row);
    auto entryCount = pageSize_ / entrySize;
    auto newPage = newPageFunc_(row, entryCount, entrySize);

    auto col = counters_[row].fetch_add(1, std::memory_order_relaxed);
    auto dataIndex = (cols_ * row) + (col % cols_);
    data_[dataIndex].exchange(newPage, std::memory_order_relaxed);

    return newPage;
}

rz::memstr::PagePtr rz::memstr::PageNursery::Get( colcount_t col, rowcount_t row) {
    auto dataIndex = (cols_ * row) + (col % cols_);
    auto page = data_[dataIndex];
    if (!page) {
        page = New(row);
    }

    return page;
}

rz::memstr::PagePtr rz::memstr::PageNursery::Get( colcount_t col, rowcount_t row) const {
    auto dataIndex = (cols_ * row) + (col % cols_);
    auto page = data_[dataIndex];
    return page;
}

rz::memstr::PageNursery::Iterator rz::memstr::PageNursery::Iter( rowcount_t row) const noexcept {
    Iterator iter;

    if (row < rows_) {
        auto start = counters_[row].load(std::memory_order_relaxed);
        start = start > cols_ ? (start % cols_) : 0;
        iter = Iterator{cols_, row, start};
    }

    return iter;
}

rz::memstr::PagePtr rz::memstr::PageNursery::Next( Iterator& iter) {
    if (!!iter) {
        auto col = iter++;
        return Get(col, iter.row_);
    } else {
        return PagePtr{};
    }
}

std::vector<rz::memstr::PagePtr> rz::memstr::PageNursery::GetPages( rowcount_t row) const {
    std::vector<PagePtr> pages;

    auto cols = counters_[row].load(std::memory_order_relaxed);
    cols = cols > cols_ ? cols_ : cols;
    for (decltype(cols) i = 0; i < cols; ++i) {
        auto page = Get(i, row);
        if (!!page) {
            pages.emplace_back(std::move(page));
        }
    }

    return pages;
}

std::vector<rz::memstr::Page::entrycount_t> rz::memstr::PageNursery::GetPageEntryCounts( rowcount_t row) const {
    std::vector<Page::entrycount_t> entries;

    auto pages = GetPages(row);
    for (auto page : pages) {
        if (!!page) {
            auto count = page->GetEntryCount();
            entries.push_back(count);
        }
    }

    return entries;
}
