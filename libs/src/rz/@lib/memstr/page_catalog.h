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

#ifndef MEMSTR_PAGE_CATALOG_H
#define	MEMSTR_PAGE_CATALOG_H

#include <atomic>
#include <vector>

#include "page.h"
#include "hash.h"
#include "reference.h"

namespace rz {
namespace memstr {

class PageCatalog final {
public:

    using rowcount_t = std::uint32_t;
    using colcount_t = std::uint32_t;
    using pagecount_t = std::uint32_t;

	PageCatalog( colcount_t, rowcount_t );
	PageCatalog( const PageCatalog & ) = delete;
	PageCatalog( const PageCatalog && ) = delete;
	void operator=( const PageCatalog & ) = delete;

    bool Add( rowcount_t, PagePtr );

    RefString Find( rowcount_t, StringHash::Hash ) const noexcept;

    inline rowcount_t Rows() const noexcept { return rows_; }
    inline colcount_t Cols() const noexcept { return cols_; }

    inline pagecount_t GetPageCount() const noexcept { return totalPages_.load( std::memory_order_relaxed ); }

    std::vector<PagePtr> GetPages( rowcount_t ) const;

private:

    std::vector<std::atomic<rowcount_t>> counters_;
    std::vector<PagePtr> data_;

    const colcount_t cols_;
    const rowcount_t rows_;

    std::atomic<pagecount_t> totalPages_;
};

}}

#endif	/* MEMSTR_PAGE_CATALOG_H */

