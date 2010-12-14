// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef DIADEM_METRICS_H_
#define DIADEM_METRICS_H_

#include "Diadem/Entity.h"

namespace Diadem {

struct Spacing {
  long top, left, bottom, right;

  Spacing(long t, long l, long b, long r)
      : top(t), left(l), bottom(b), right(r) {}
  Spacing()
      : top(0), left(0), bottom(0), right(0) {}

  Bool operator!=(const Spacing &s) const {
    return (top != s.top) || (left != s.left) ||
        (bottom != s.bottom) || (right != s.right);
  }
  Spacing operator+(const Spacing &s) const
    { return Spacing(top+s.top, left+s.left, bottom+s.bottom, right+s.right); }
  Spacing operator-(const Spacing &s) const
    { return Spacing(top-s.top, left-s.left, bottom-s.bottom, right-s.right); }
  Spacing& operator+=(const Spacing &s) {
    top += s.top;
    left += s.left;
    bottom += s.bottom;
    right += s.right;
    return *this;
  }
};

struct Size {
  long width, height;

  Size(long w, long h) : width(w), height(h) {}
  Size() : width(0), height(0) {}

  Bool operator==(const Size &s) const
    { return (width == s.width) && (height == s.height); }
  Bool operator!=(const Size &s) const
    { return (width != s.width) || (height != s.height); }
  Size operator+(const Spacing &s) const
    { return Size(width + (s.left + s.right), height + (s.top + s.bottom)); }
  Size operator-(const Spacing &s) const
    { return Size(width - (s.left + s.right), height - (s.top + s.bottom)); }
};

struct Location {
  long x, y;

  Location(long x1, long y1) : x(x1), y(y1) {}
  Location() : x(0), y(0) {}

  Bool operator==(const Location &l) const
    { return (x == l.x) && (y == l.y); }
  Bool operator!=(const Location &l) const
    { return (x != l.x) || (y != l.y); }

  Location operator+(const Location &l) const
    { return Location(x+l.x, y+l.y); }
  Location operator-(const Location &l) const
    { return Location(x-l.x, y-l.y); }
  Location& operator-=(const Location &l) {
    x -= l.x;
    y -= l.y;
    return *this;
  }
  Location& operator+=(const Location &l) {
    x += l.x;
    y += l.y;
    return *this;
  }
  Location operator-() const
    { return Location(-x, -y); }
};

struct PlatformMetrics {
  uint32_t em_size, line_height, indent_size;
  Spacing radio_group_padding;
};

enum Unit {
  kUnitPixels,
  kUnitEms,
  kUnitLines,
  kUnitIndent,
};

class ExplicitSize {
 public:
  float width_, height_;
  Unit width_units_, height_units_;

  ExplicitSize()
    : width_(0), height_(0),
      width_units_(kUnitPixels), height_units_(kUnitPixels) {}
  explicit ExplicitSize(const Size &size)
    : width_(size.width), height_(size.height),
      width_units_(kUnitPixels), height_units_(kUnitPixels) {}

  int32_t CalculateWidth(const PlatformMetrics &metrics) const {
    switch (width_units_) {
      case kUnitIndent: return width_ * metrics.indent_size;
      case kUnitEms:    return width_ * metrics.em_size;
      case kUnitPixels:
      default:          return width_;
    }
  }
  int32_t CalculateHeight(const PlatformMetrics &metrics) const {
    switch (height_units_) {
      case kUnitLines:  return height_ * metrics.line_height;
      case kUnitEms:    return height_ * metrics.em_size;
      case kUnitPixels:
      default:          return height_;
    }
  }
};

}  // namespace Diadem

#endif  // DIADEM_METRICS_H_
