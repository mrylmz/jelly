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

#include <llvm/Support/Path.h>

#include "Basic/SourceBuffer.h"
#include "Basic/SourceManager.h"

using namespace jelly;

SourceBuffer SourceManager::getSourceBuffer(unsigned bufferId) {
    if (bufferId > 0) {
        auto buffer = manager.getMemoryBuffer(bufferId);
        return SourceBuffer(buffer->getBufferStart(), buffer->getBufferEnd(), bufferId);
    }
    return SourceBuffer(nullptr, nullptr, 0);
}

SourceBuffer SourceManager::addSourceBuffer(StringRef source, SourceLocation includeLocation) {
    auto buffer = llvm::MemoryBuffer::getMemBuffer(source, "", false);
    auto bufferId = manager.AddNewSourceBuffer(std::move(buffer), includeLocation.value);
    return getSourceBuffer(bufferId);
}

SourceBuffer SourceManager::addIncludeFile(StringRef filePath, SourceLocation location) {
    std::string fullFilePath;
    auto bufferId = manager.AddIncludeFile(filePath, location.value, fullFilePath);
    return getSourceBuffer(bufferId);
}

SourceBuffer SourceManager::getBufferContaining(SourceLocation location) {
    auto bufferId = manager.FindBufferContainingLoc(location.value);
    return getSourceBuffer(bufferId);
}

void SourceManager::setWorkingDirectory(StringRef directory) {
    llvm::sys::fs::set_current_path(directory);
}
