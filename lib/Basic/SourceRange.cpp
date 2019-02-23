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

#include "Basic/SourceRange.h"

using namespace jelly;

SourceRange::SourceRange() {
}

SourceRange::SourceRange(SourceLocation location) :
start(location),
end(location) {
}

SourceRange::SourceRange(SourceLocation start, SourceLocation end) :
start(start),
end(end) {
}

bool SourceRange::isValid() const {
    return start.isValid() && end.isValid();
}

bool SourceRange::contains(SourceLocation location) const {
    return start <= location && location <= end;
}

bool SourceRange::operator == (const SourceRange &rhs) const {
    return start == rhs.start && end == rhs.end;
}

bool SourceRange::operator != (const SourceRange &rhs) const {
    return start != rhs.start || end != rhs.end;
}
