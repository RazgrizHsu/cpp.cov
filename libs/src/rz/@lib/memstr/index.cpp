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

#include "idx.h"

#include "hash.h"

#include <type_traits>

template<typename T> static constexpr std::size_t GetBufferValueSizeMultiple() {
    return sizeof(typename std::remove_reference<T>::type::value_type) / sizeof(rz::memstr::Page::buffervalue_t);
}

rz::memstr::Store::Store() {

}

rz::memstr::RefString rz::memstr::Store::Add(const char* str) {
    return pages_.Add(str);
}

rz::memstr::RefString rz::memstr::Store::Add(const std::string& str) {
    auto pStr = str.c_str();
    auto len = str.length();
    auto hash = StringHash::Get(pStr, len);
    return pages_.Add(pStr, len, hash);
}

rz::memstr::RefString rz::memstr::Store::Add(const std::wstring& str) {
    auto pStr = reinterpret_cast<Page::const_bufferptr_t>(str.c_str());
    auto len = str.length() * GetBufferValueSizeMultiple<decltype(str)>();
    auto hash = StringHash::Get(pStr, len);
    return pages_.Add(pStr, len, hash);
}

rz::memstr::RefString rz::memstr::Store::Add(const std::u16string& str) {
    auto pStr = reinterpret_cast<Page::const_bufferptr_t>(str.c_str());
    auto len = str.length() * GetBufferValueSizeMultiple<decltype(str)>();
    auto hash = StringHash::Get(pStr, len);
    return pages_.Add(pStr, len, hash);
}

rz::memstr::RefString rz::memstr::Store::Add(const std::u32string& str) {
    auto pStr = reinterpret_cast<Page::const_bufferptr_t>(str.c_str());
    auto len = str.length() * GetBufferValueSizeMultiple<decltype(str)>();
    auto hash = StringHash::Get(pStr, len);
    return pages_.Add(pStr, len, hash);
}

const char* rz::memstr::Store::ToString(const RefString& ref) const {
    return pages_.GetString(ref);
}

bool rz::memstr::Store::ToString(const RefString& ref, std::string& str) const {
    auto status = false;
    auto pStr = pages_.GetString(ref);
    if (!!pStr) {
        str = std::move(std::string(pStr));
        status = true;
    }
    return status;
}

bool rz::memstr::Store::ToString(const RefString& ref, std::wstring& str) const {
    auto status = false;
    Page::entrysize_t entrySize;
    auto pStr = reinterpret_cast<std::remove_reference<decltype(str)>::type::const_pointer>(pages_.GetString(ref, entrySize));
    if (!!pStr) {
        entrySize /= GetBufferValueSizeMultiple<decltype(str)>();
        str = std::move(std::wstring(pStr, entrySize));
        status = true;
    }
    return status;
}

bool rz::memstr::Store::ToString(const RefString& ref, std::u16string& str) const {
    auto status = false;
    Page::entrysize_t entrySize;
    auto pStr = reinterpret_cast<std::remove_reference<decltype(str)>::type::const_pointer>(pages_.GetString(ref, entrySize));
    if (!!pStr) {
        entrySize /= GetBufferValueSizeMultiple<decltype(str)>();
        str = std::move(std::u16string(pStr, entrySize));
        status = true;
    }
    return status;
}

bool rz::memstr::Store::ToString(const RefString& ref, std::u32string& str) const {
    auto status = false;
    Page::entrysize_t entrySize;
    auto pStr = reinterpret_cast<std::remove_reference<decltype(str)>::type::const_pointer>(pages_.GetString(ref, entrySize));
    if (!!pStr) {
        entrySize /= GetBufferValueSizeMultiple<decltype(str)>();
        str = std::move(std::u32string(pStr, entrySize));
        status = true;
    }
    return status;
}

rz::memstr::StringHash::Hash rz::memstr::Store::GetHash(const RefString& ref) const noexcept {
    return pages_.GetHash(ref);
}

bool rz::memstr::Store::Compare(const RefString& lhs, const RefString& rhs) const noexcept {
    return pages_.Compare(lhs, rhs);
}
