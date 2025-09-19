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

#ifndef MEMSTR_PAGES_H
#define	MEMSTR_PAGES_H

#include <atomic>
#include <cstddef>

#include "reference.h"
#include "page_nursery.h"
#include "page_catalog.h"
#include "page_archive.h"

namespace rz {
namespace memstr {

class Pages final {

public:
    Pages();

    RefString Add(const char*);
    RefString Add(const char*, std::size_t, StringHash::Hash);

    const char* GetString(const RefString&) const;
    const char* GetString( const RefString&, Page::entrysize_t&) const;

    StringHash::Hash GetHash(const RefString&) const;

    bool Compare(const RefString&, const RefString&) const noexcept;

    std::size_t GetPageCount() const noexcept { return archive_.GetPageCount(); }
    std::size_t GetEntryCount() const noexcept { return archive_.GetEntryCount(); }

#ifdef MEMSTRPAGES_INTERNALSTATE
    const StringPageArchive& GetArchive() const noexcept { return archive_; }
    const StringPageNursery& GetNursery() const noexcept { return nursery_; }
    const StringPageCatalog& GetCatalog() const noexcept { return catalog_; }
    StringPageNursery::colcount_t GetNurseryCols() const noexcept { return nurseryCols_; }
#endif

private:

    PagePtr NewPage( PageNursery::rowcount_t, Page::entrycount_t, Page::entrysize_t);

    constexpr static PageNursery::colcount_t nurseryCols_ = 8;
    constexpr static PageNursery::pagesize_t stringPageSizeBytes_ = 2 * 1024 * 1024;
    constexpr static PageNursery::colcount_t catalogCols_ = 4096;

    PageArchive archive_;
    PageNursery nursery_;
    PageCatalog catalog_;
};

}}

#endif	/* MEMSTR_PAGES_H */

