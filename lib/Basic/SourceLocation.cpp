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

#include "Basic/SourceLocation.h"

using namespace jelly;

SourceLocation::SourceLocation(llvm::SMLoc value) :
value(value) {
}

SourceLocation::SourceLocation(const char* pointer) :
value(llvm::SMLoc::getFromPointer(pointer)) {
}

const char* SourceLocation::getPointer() const {
    return value.getPointer();
}

SourceLocation::SourceLocation() {
}

bool SourceLocation::isValid() const {
    return value.isValid();
}

bool SourceLocation::isBefore(SourceLocation rhs) const {
    return getPointer() < rhs.getPointer();
}

bool SourceLocation::isAfter(SourceLocation rhs) const {
    return getPointer() > rhs.getPointer();
}

SourceLocation SourceLocation::getAdvancedLocation(int offset) const {
    assert(isValid());
    return SourceLocation(getPointer() + offset);
}

bool SourceLocation::operator == (const SourceLocation& rhs) const {
    return value == rhs.value;
}

bool SourceLocation::operator != (const SourceLocation& rhs) const {
    return value != rhs.value;
}

bool SourceLocation::operator <  (const SourceLocation& rhs) const {
    return getPointer() < rhs.getPointer();
}

bool SourceLocation::operator <= (const SourceLocation& rhs) const {
    return getPointer() <= rhs.getPointer();
}

bool SourceLocation::operator >  (const SourceLocation& rhs) const {
    return getPointer() > rhs.getPointer();
}

bool SourceLocation::operator >= (const SourceLocation& rhs) const {
    return getPointer() >= rhs.getPointer();
}
