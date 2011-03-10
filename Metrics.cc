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

#include "Diadem/Metrics.h"
#include "Diadem/Layout.h"  // for FirstLetter()

namespace Diadem {

Spacing Spacing::Union(const Spacing &a, const Spacing &b) {
  return Spacing(
      std::max(a.top, b.top), std::max(a.left, b.left),
      std::max(a.bottom, b.bottom), std::max(a.right, b.right));
}

void ExplicitSize::ParseWidth(const char *value) {
  if (strcmp(value, "indent") == 0) {
    width_ = 1;
    width_units_ = kUnitIndent;
  } else {
    const char *letters = FirstLetter(value);

    if ((letters != NULL) && (strcmp(letters, "em") == 0))
      width_units_ = kUnitEms;
    else
      width_units_ = kUnitPixels;
    width_ = strtof(value, NULL);
  }
}

void ExplicitSize::ParseHeight(const char *value) {
  const char *letters = FirstLetter(value);

  if ((letters != NULL) && (strcmp(letters, "li") == 0))
    height_units_ = kUnitLines;
  else
    height_units_ = kUnitPixels;
  height_ = strtof(value, NULL);
}

int32_t ExplicitSize::CalculateWidth(const PlatformMetrics &metrics) const {
  switch (width_units_) {
    case kUnitIndent: return width_ * metrics.indent_size;
    case kUnitEms:    return width_ * metrics.em_size;
    case kUnitPixels:
    default:          return width_;
  }
}

int32_t ExplicitSize::CalculateHeight(const PlatformMetrics &metrics) const {
  switch (height_units_) {
    case kUnitLines:  return height_ * metrics.line_height;
    case kUnitEms:    return height_ * metrics.em_size;
    case kUnitPixels:
    default:          return height_;
  }
}

}  // namespace Diadem
