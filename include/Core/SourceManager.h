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

#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Path.h>

class SourceManager;

class SourceLocation {
    friend class SourceManager;

    llvm::SMLoc value;

    SourceLocation(llvm::SMLoc value) : value(value) {}
    SourceLocation(const char* pointer) : value(llvm::SMLoc::getFromPointer(pointer)) {}

    const char* getPointer() const {
        return value.getPointer();
    }

public:
    SourceLocation() {}

    bool isValid() const { return value.isValid(); }
    bool isBefore(SourceLocation rhs) const { return getPointer() < rhs.getPointer(); }
    bool isAfter(SourceLocation rhs) const { return getPointer() > rhs.getPointer(); }

    SourceLocation getAdvancedLocation(int offset) const {
        assert(isValid());
        return SourceLocation(getPointer() + offset);
    }

    bool operator == (const SourceLocation& rhs) const { return value == rhs.value; }
    bool operator != (const SourceLocation& rhs) const { return value != rhs.value; }
    bool operator <  (const SourceLocation& rhs) const { return getPointer() < rhs.getPointer(); }
    bool operator <= (const SourceLocation& rhs) const { return getPointer() <= rhs.getPointer(); }
    bool operator >  (const SourceLocation& rhs) const { return getPointer() > rhs.getPointer(); }
    bool operator >= (const SourceLocation& rhs) const { return getPointer() >= rhs.getPointer(); }
};

class SourceRange {
    SourceLocation start;
    SourceLocation end;

public:
    SourceRange() {}
    SourceRange(SourceLocation location) : start(location), end(location) {}
    SourceRange(SourceLocation start, SourceLocation end) : start(start), end(end) {}

    bool isValid() const { return start.isValid() && end.isValid(); }
    bool contains(SourceLocation location) const { return start <= location && location <= end; }

    bool operator == (const SourceRange &rhs) const { return start == rhs.start && end == rhs.end; }
    bool operator != (const SourceRange &rhs) const { return start != rhs.start || end != rhs.end; }
};

class SourceBuffer {
    friend class SourceManager;

    const char* bufferStart;
    const char* bufferEnd;
    unsigned bufferId;

    SourceBuffer(const char* bufferStart, const char* bufferEnd, unsigned bufferId) :
    bufferStart(bufferStart),
    bufferEnd(bufferEnd),
    bufferId(bufferId) {}

public:
    bool isValid() const { return bufferId > 0; }

    const char *getBufferStart() const { return bufferStart; }
    const char *getBufferEnd() const { return bufferEnd; }

    size_t getBufferSize() const { return bufferEnd - bufferStart; }

    llvm::StringRef getBuffer() const { return llvm::StringRef(bufferStart, getBufferSize()); }
};

class SourceManager {
    llvm::SourceMgr manager;

    SourceBuffer getSourceBuffer(unsigned bufferId) {
        if (bufferId > 0) {
            auto buffer = manager.getMemoryBuffer(bufferId);
            return SourceBuffer(buffer->getBufferStart(), buffer->getBufferEnd(), bufferId);
        }
        return SourceBuffer(nullptr, nullptr, 0);
    }

public:
    SourceBuffer addSourceBuffer(llvm::StringRef source, SourceLocation includeLocation = {}) {
        auto buffer = llvm::MemoryBuffer::getMemBuffer(source, "", false);
        auto bufferId = manager.AddNewSourceBuffer(std::move(buffer), includeLocation.value);
        return getSourceBuffer(bufferId);
    }

    SourceBuffer addIncludeFile(llvm::StringRef filePath, SourceLocation location = {}) {
        std::string fullFilePath;
        auto bufferId = manager.AddIncludeFile(filePath, location.value, fullFilePath);
        return getSourceBuffer(bufferId);
    }

    SourceBuffer getBufferContaining(SourceLocation location) {
        auto bufferId = manager.FindBufferContainingLoc(location.value);
        return getSourceBuffer(bufferId);
    }

    static void setWorkingDirectory(llvm::StringRef directory) {
        llvm::sys::fs::set_current_path(directory);
    }
};
