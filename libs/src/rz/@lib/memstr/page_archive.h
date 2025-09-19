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

#ifndef MEMSTR_PAGE_ARCHIVE_H
#define	MEMSTR_PAGE_ARCHIVE_H

#include <atomic>
#include <vector>
#include <cstdint>

#include "page.h"

namespace rz {
namespace memstr {

class PageArchive final {
public:
    PageArchive( std::size_t entries);
    PageArchive( const PageArchive& orig) = delete;
    PageArchive( PageArchive&&) = delete;
    void operator=(const PageArchive&) = delete;

    PagePtr NewPage( Page::entrycount_t entryCount, Page::entrysize_t entrySize);
    PagePtr NewPage( Page::bufferptr_t ptr, Page::entrycount_t entryCount, Page::entrysize_t entrySize);

    PagePtr GetPage( std::size_t number) const noexcept;

    std::size_t GetPageCount() const noexcept;
    std::size_t GetEntryCount() const noexcept;

    std::size_t GetMaxPages() const noexcept { return pages_.size(); }

private:

    std::atomic<std::size_t> index_;

    std::vector<PagePtr> pages_;

};

}}

#endif	/* MEMSTR_PAGE_ARCHIVE_H */
