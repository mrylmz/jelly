//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#define jelly_unreachable llvm_unreachable

namespace jelly {
    using llvm::APInt;
    using llvm::ArrayRef;
    using llvm::MapVector;
    using llvm::StringMap;
    using llvm::StringRef;
    using llvm::SmallVector;
    using llvm::MallocAllocator;
    using llvm::BumpPtrAllocator;
    using llvm::fatal_error_handler_t;
    using llvm::install_fatal_error_handler;
    using llvm::remove_fatal_error_handler;
    using llvm::report_fatal_error;
    using llvm::install_bad_alloc_error_handler;
    using llvm::remove_bad_alloc_error_handler;
    using llvm::report_bad_alloc_error;
    using llvm::makeArrayRef;
    using llvm::sys::path::native;
    using llvm::formatv;
    using llvm::format_provider;
}
