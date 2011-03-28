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

#include "Diadem/Layout.h"

#include <algorithm>

#include "Diadem/Native.h"
#include "Diadem/Value.h"

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFF
#endif

namespace Diadem {

const PropertyName
    kPropSize         = "size",
    kPropLocation     = "loc",
    kPropMinimumSize  = "minsize",
    kPropWidthOption  = "width",
    kPropHeightOption = "height",
    kPropMaxWidth     = "maxWidth",
    kPropMaxHeight    = "maxHeight",
    kPropWidthName    = "widthName",
    kPropHeightName   = "heightName",
    kPropAlign        = "align",
    kPropVisible      = "visible",
    kPropInLayout     = "inLayout",
    kPropDirection    = "direction",
    kPropAmount       = "amount",
    kPropPadding      = "padding",
    kPropMargins      = "margins",
    kPropBaseline     = "baseline";

const TypeName
    kTypeNameGroup  = "group",
    kTypeNameMulti  = "multi",
    kTypeNameSpacer = "spacer";

const StringConstant
    kSizeNameDefault = "default",
    kSizeNameFit     = "fit",
    kSizeNameFill    = "fill";

const StringConstant
    kAlignNameStart  = "start",
    kAlignNameCenter = "center",
    kAlignNameEnd    = "end";

const StringConstant
    kDirectionNameRow    = "row",
    kDirectionNameColumn = "column";

// Height or width may be specified as fit, fill or default, or an explicit
// size. Fit is the smallest size that will fit the object's contents. Fill
// expands to take up any extra space in the parent container. Default will
// vary depending on the object type.
static bool ParseSizeOption(const char *c, SizeOption *size) {
  const char* strings[3] = {
      kSizeNameDefault, kSizeNameFit, kSizeNameFill };
  const SizeOption options[3] = { kSizeDefault, kSizeFit, kSizeFill };

  DASSERT(size != NULL);
  for (int i = 0; i < 3; ++i)
    if (strcmp(c, strings[i]) == 0) {
      *size = options[i];
      return true;
    }
  return false;
}

const char *FirstLetter(const char *s) {
  const char *l = s;

  for (; !isalpha(*l); ++l) {
    if (*l == '\0')
      return NULL;
  }
  return l;
}

// Reverses an alignment value for RTL layout
static AlignOption ReverseAlignment(AlignOption a) {
  switch (a) {
    case kAlignStart:  return kAlignEnd;
    case kAlignCenter: return kAlignCenter;
    case kAlignEnd:    return kAlignStart;
  }
  return kAlignStart;
}

void Layout::InitializeProperties(const PropertyMap &properties) {
  const Array<String> keys = properties.AllKeys();

  for (size_t i = 0; i < keys.size(); ++i)
    SetProperty(keys[i], properties[keys[i]]);
}

Layout* Layout::GetLayoutParent() {
  if ((entity_ == NULL) || (entity_->GetParent() == NULL))
    return NULL;
  return entity_->GetParent()->GetLayout();
}

const Layout* Layout::GetLayoutParent() const {
  if ((entity_ == NULL) || (entity_->GetParent() == NULL))
    return NULL;
  return entity_->GetParent()->GetLayout();
}

void Layout::InvalidateLayout() {
  Layout *parent = GetLayoutParent();

  if (parent != NULL)
    parent->InvalidateLayout();
}

Layout::LayoutDirection Layout::GetDirection() const {
  const Layout *parent = GetLayoutParent();

  if (parent == NULL)
    return kLayoutRow;
  return parent->GetDirection();
}

bool Layout::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropInLayout) == 0) {
    in_layout_ = value.Coerce<bool>();
    entity_->SetProperty(kPropVisible, in_layout_);
    InvalidateLayout();
    return true;
  }
  if (strcmp(name, kPropWidthOption) == 0) {
    const String width_string = value.Coerce<String>();

    if (!ParseSizeOption(width_string, &h_size_)) {
      explicit_size_.ParseWidth(width_string);
      h_size_ = kSizeExplicit;
    }
    return true;
  }
  if (strcmp(name, kPropHeightOption) == 0) {
    const String height_string = value.Coerce<String>();

    if (!ParseSizeOption(height_string, &v_size_)) {
      explicit_size_.ParseHeight(height_string);
      v_size_ = kSizeExplicit;
    }
    return true;
  }
  if (strcmp(name, kPropAlign) == 0) {
    const String align_string = value.Coerce<String>();

    if (align_string == kAlignNameStart)
      align_ = kAlignStart;
    else if (align_string == kAlignNameCenter)
      align_ = kAlignCenter;
    else if (align_string == kAlignNameEnd)
      align_ = kAlignEnd;
    return true;
  }
  if (strcmp(name, kPropWidthName) == 0) {
    width_name_ = value.Coerce<String>();
  }
  if (strcmp(name, kPropHeightName) == 0) {
    height_name_ = value.Coerce<String>();
  }
  return false;
}

Value Layout::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropInLayout) == 0) {
    return Value(in_layout_);
  }
  if (strcmp(name, kPropWidthOption) == 0) {
    return Value(static_cast<int>(h_size_));
  }
  return Value();
}

Location Layout::GetViewLocation() const {
  const Native* const native = entity_->GetNative();

  // If the native object is a superview, then it defines the origin for its
  // subviews, so its location is (0, 0)
  if ((native != NULL) && (native->IsSuperview()))
    return Location();
  if (entity_->GetParent() != NULL) {
    const Layout* const parent_layout = entity_->GetParent()->GetLayout();

    if (parent_layout != NULL)
      return GetLocation() + parent_layout->GetViewLocation();
  }
  return GetLocation();
}

void Layout::ParentLocationChanged(const Location &offset) {
  if (offset != Location()) {
    const Value old_location = entity_->GetProperty(kPropLocation);

    if (old_location.IsValid())
      entity_->SetProperty(
          kPropLocation, old_location.Coerce<Location>() + offset);
  }
}

Size Layout::CalculateMinimumSize() const {
  return entity_->GetProperty(kPropMinimumSize).Coerce<Size>();
}

Size Layout::EnforceExplicitSize(const Size &size) const {
  Size result = size;
  const PlatformMetrics &metrics = GetPlatformMetrics();

  if (!width_name_.IsEmpty()) {
    result.width = FindWidthForName();
  } else if (h_size_ == kSizeExplicit) {
    uint32_t h_multiplier = 1;

    switch (explicit_size_.width_units_) {
      case kUnitEms:
        h_multiplier = metrics.em_size;
        break;
      case kUnitIndent:
        h_multiplier = metrics.indent_size;
        break;
      default:
        break;
    }
    result.width =
        std::max<int32_t>(size.width, explicit_size_.width_ * h_multiplier);
  }
  if (!height_name_.IsEmpty()) {
    result.height = FindHeightForName();
  } else if (v_size_ == kSizeExplicit) {
    uint32_t v_multiplier = 1;

    switch (explicit_size_.width_units_) {
      case kUnitEms:
        v_multiplier = metrics.em_size;
        break;
      case kUnitIndent:
        v_multiplier = metrics.indent_size;
        break;
      case kUnitLines:
        v_multiplier = metrics.line_height;
      default:
        break;
    }
    result.height =
        std::max<int32_t>(size.height, explicit_size_.height_ * v_multiplier);
  }
  return result;
}

uint32_t Layout::FindWidthForName() const {
  const Entity *root = entity_;

  for (; root->GetParent() != NULL; root = root->GetParent()) {
    // Non-layout entities should only be leaf nodes in the hierarchy
    DASSERT(root->GetLayout() != NULL);
  }
  return root->GetLayout()->FindDimensionForName(
      kDimensionWidth, width_name_);
}

uint32_t Layout::FindHeightForName() const {
  const Entity *root = entity_;

  for (; root->GetParent() != NULL; root = root->GetParent()) {
    // Non-layout entities should only be leaf nodes in the hierarchy
    DASSERT(root->GetLayout() != NULL);
  }
  return root->GetLayout()->FindDimensionForName(
      kDimensionHeight, width_name_);
}

uint32_t Layout::FindDimensionForName(
    Dimension dimension, const String &name) const {
  if (dimension == kDimensionWidth) {
    if (name == width_name_)
      return CalculateMinimumSize().width;
  } else {  // kDimensionHeight
    if (name == height_name_)
      return CalculateMinimumSize().height;
  }

  uint32_t max = 0;

  for (size_t i = 0; i < entity_->ChildrenCount(); ++i) {
    const Layout *layout = entity_->ChildAt(i)->GetLayout();

    if (layout == NULL)
      continue;
    max = std::max(max, layout->FindDimensionForName(dimension, name));
  }
  return max;
}

void Layout::SetInLayout(bool in_layout) {
  in_layout_ = in_layout;
  SetVisible(in_layout);
}

const PlatformMetrics& Layout::GetPlatformMetrics() const {
  for (const Entity *e = entity_; e != NULL; e = e->GetParent()) {
    const Native *native = e->GetNative();

    if (native != NULL)
      return native->GetPlatformMetrics();
  }

  static PlatformMetrics no_metrics = {};

  DASSERT(false);  // Platform metrics must be somewhere in the hierarchy.
  return no_metrics;
}

bool LayoutContainer::SetProperty(const char *name, const Value &value) {
  if (strcmp(name, kPropDirection) == 0) {
    if (value.IsValueType<String>()) {
      const String direction_string = value.Coerce<String>();

      if (direction_string == kDirectionNameRow)
        direction_ = kLayoutRow;
      else if (direction_string == kDirectionNameColumn)
        direction_ = kLayoutColumn;
      return true;
    } else {
      direction_ = (LayoutDirection)value.Coerce<int32_t>();
      return true;
    }
  }
  if (strcmp(name, kPropVisible) == 0) {
    visible_ = value.Coerce<bool>();
    // TODO(catmull): hide children
    return true;
  }
  return Layout::SetProperty(name, value);
}

Value LayoutContainer::GetProperty(const char *name) const {
  if (strcmp(name, kPropDirection) == 0) {
    return direction_;
  }
  if (strcmp(name, kPropVisible) == 0) {
    return visible_;
  }
  if (strcmp(name, kPropMargins) == 0) {
    return GetMargins();
  }
  return Layout::GetProperty(name);
}

Spacing LayoutContainer::GetMargins() const {
  const Value native_margins = entity_->GetNativeProperty(kPropMargins);

  if (native_margins.IsValid())
    return native_margins.Coerce<Spacing>();
  return Spacing();
}

int32_t LayoutContainer::GetBaseline() const {
  if (entity_->ChildrenCount() > 0)
    return entity_->ChildAt(0)->GetProperty(kPropBaseline).Coerce<int32_t>();
  return Layout::GetBaseline();
}

void LayoutContainer::InvalidateLayout() {
  cached_min_size_ = Size();
  Layout::InvalidateLayout();
}

void LayoutContainer::SetSize(const Size &s) {
  if (s == GetSize())
    return;

  Size new_size;
  uint32_t extra;

  // Resizing a container is done in two phases: setting the sizes of the
  // children, and then setting their locations.
  SetObjectSizes(s, &new_size, &extra);
  ArrangeObjects(new_size, extra);
}

void LayoutContainer::SetLocation(const Location &loc) {
  const Location current = GetLocation();
  const Location offset(loc.x-current.x, loc.y-current.y);

  SetLocationImp(loc);

  // If the native object is a superview, then subviews will be moved
  // automatically. Otherwise, children need to be notified of the change.
  if ((entity_->GetNative() == NULL) || (!entity_->GetNative()->IsSuperview()))
    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
      Layout *layout_child = entity_->ChildAt(i)->GetLayout();

      if (layout_child != NULL)
        layout_child->ParentLocationChanged(offset);
    }
}

void LayoutContainer::ParentLocationChanged(const Location &offset) {
  if (offset != Location())
    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
      Layout *layout_child = entity_->ChildAt(i)->GetLayout();

      if (layout_child != NULL)
        layout_child->ParentLocationChanged(offset);
    }
}

uint32_t LayoutContainer::FillChildCount() const {
  uint32_t count = 0;

  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
    const Layout* const child = entity_->ChildAt(i)->GetLayout();

    if ((child == NULL) || !child->IsInLayout())
      continue;
    if (StreamSizeOption(*child) == kSizeFill)
      ++count;
  }
  return count;
}

static const uint32_t kMaxLayoutIterations = 3;

void LayoutContainer::SetObjectSizes(
    const Size &s, Size *new_size, uint32_t *extra) {
  const Spacing margins = GetMargins();
  const unsigned int fill_count = FillChildCount();
  bool layout_valid = false;

  // For some objects, such as wrapped text, changing their size in one
  // direction will affect their minimum size in the other. When that happens,
  // the object will call InvalidateLayout, and the loop will be executed again.
  for (uint32_t i = 0; !layout_valid && (i < kMaxLayoutIterations); ++i) {
    layout_valid = true;
    if (cached_min_size_ == Size())
      cached_min_size_ = GetMinimumSize();

    *new_size = Size(
        std::max(s.width, cached_min_size_.width),
        std::max(s.height, cached_min_size_.height));
    *extra = ExtraSpace(StreamDim(*new_size));

    SetSizeImp(*new_size);

    const uint32_t fill = (fill_count == 0) ? 0 : (*extra / fill_count);
    uint32_t remainder  = (fill_count == 0) ? 0 : (*extra % fill_count);

    for (uint32_t j = 0; j < entity_->ChildrenCount(); ++j) {
      Layout* const child = entity_->ChildAt(j)->GetLayout();

      if ((child == NULL) || !child->IsInLayout())
        continue;

      const Size min = child->GetMinimumSize();
      const Size max = child->GetMaximumSize();
      const Spacing padding = child->GetPadding();
      Size size = child->GetSize();

      if ((CrossSizeOption(*child) == kSizeFill) ||
          (CrossDim(max) == kSizeFill))
        CrossDim(size) = CrossDim(*new_size) -
            (CrossBefore(margins)+CrossAfter(margins));
      else
        CrossDim(size) = CrossDim(min);

      if (fill_count == 0) {
        StreamDim(size) = StreamDim(min);
      } else {  // Distribute the extra space among the fill objects
        if ((StreamSizeOption(*child) == kSizeFill) ||
            (StreamDim(max) == kSizeFill)) {
          int32_t item_fill = fill + StreamDim(min);

          if (remainder > 0) {
            ++item_fill;
            --remainder;
          }
          StreamDim(size) = item_fill;
        } else {
          StreamDim(size) = StreamDim(min);
        }
      }
      child->SetSize(size);
      // With wrapped text, changing the width may change the desired height.
      if (child->GetMinimumSize() != min)
        layout_valid = false;
    }
    if (!layout_valid) {
      cached_min_size_ = Size();
      CalculateMinimumSize();
    }
  }
  if (fill_count != 0)
    *extra = 0;
}

void LayoutContainer::ArrangeObjects(const Size &new_size, uint32_t extra) {
  if (entity_->ChildrenCount() == 0)
    return;

  const Spacing margins = GetMargins();
  int32_t prev_pad = StreamBefore(margins);
  int32_t last_edge = 0;
  const bool reverse_row = (direction_ == kLayoutRow) && IsRTL();
  const bool reverse_col = (direction_ == kLayoutColumn) && IsRTL();

  switch (reverse_row ? ReverseAlignment(stream_align_) : stream_align_) {
    case kAlignStart:
      break;
    case kAlignCenter:
      last_edge += extra / 2;
      break;
    case kAlignEnd:
      last_edge += extra;
      break;
  }

  int32_t cross_extra;
  bool first = true;
  const uint32_t end = reverse_row ? UINT32_MAX : entity_->ChildrenCount();
  const uint8_t inc = reverse_row ? -1 : 1;

  for (uint32_t i = reverse_row ? (entity_->ChildrenCount() - 1) : 0;
       i != end; i += inc) {
    Layout *child = entity_->ChildAt(i)->GetLayout();

    if ((child == NULL) || !child->IsInLayout())
      continue;

    const Spacing padding = child->GetPadding();
    const Size child_size(child->GetSize());
    Location loc;
    int32_t before = 0;

    if (first)
      before = StreamBefore(margins);
    else if ((prev_pad >= 0) && (StreamBefore(padding) >= 0))
      before = std::max(StreamBefore(padding), prev_pad);
    StreamLoc(loc) = before + last_edge;
    CrossLoc(loc)  = CrossBefore(margins);
    const AlignOption child_align = reverse_col ?
        ReverseAlignment(child->GetAlignment()) : child->GetAlignment();

    switch (child_align) {
      case kAlignStart:
        break;
      case kAlignCenter:
      case kAlignEnd:
        cross_extra = CrossDim(new_size) - CrossDim(child_size) -
            (CrossBefore(margins) + CrossAfter(margins));
        if (child_align == kAlignEnd)
          CrossLoc(loc) += cross_extra;
        else
          CrossLoc(loc) += cross_extra / 2;
        break;
    }
    child->SetLocation(loc);
    last_edge = StreamLoc(child->GetLocation()) + StreamDim(child->GetSize());
    prev_pad = StreamAfter(padding);
    first = false;
  }
  AlignBaselines();
}

void LayoutContainer::AlignBaselines() {
  if (direction_ != kLayoutRow)
    return;

  Layout *best_items[3] = { NULL, NULL, NULL };
  int32_t baselines[3] = { 0, 0, GetSize().height };
  int32_t center_height = 0;
  uint32_t i;
  Layout *child = NULL;

  for (i = 0; i < entity_->ChildrenCount(); ++i) {
    child = entity_->ChildAt(i)->GetLayout();
    if ((child == NULL) || !child->IsInLayout())
      continue;

    const int32_t baseline = child->GetBaseline();

    switch (child->GetAlignment()) {
      case kAlignStart:
        if (baseline > baselines[0]) {
          baselines[0] = baseline;
          best_items[0] = child;
        }
        break;
      case kAlignCenter:
        if (child->GetSize().height > center_height) {
          center_height = child->GetSize().height;
          baselines[1] = baseline;
          best_items[1] = child;
        }
        break;
      case kAlignEnd:
        if (child->GetBaseline() < baselines[2]) {
          baselines[2] = baseline;
          best_items[2] = child;
        }
        break;
    }
  }
  for (i = 0; i < entity_->ChildrenCount(); ++i) {
    child = entity_->ChildAt(i)->GetLayout();
    if ((child == NULL) || !child->IsInLayout())
      continue;

    const AlignOption a = child->GetAlignment();

    if (child == best_items[a])
      continue;

    const int32_t baseline = child->GetBaseline();

    if (baseline != 0) {
      Location loc(child->GetLocation());

      loc.y += baselines[a] - baseline;
      child->SetLocation(loc);
    }
  }
}

Size LayoutContainer::CalculateMinimumSize() const {
  if (cached_min_size_ != Size())
    return cached_min_size_;

  Size min_size;

  max_size_ = Size();
  if (entity_->ChildrenCount() != 0) {
    const Spacing margins = GetMargins();
    int32_t prev_padding = 0;
    const Layout* const first_child = entity_->ChildAt(0)->GetLayout();

    const int32_t first_padding = StreamBefore(first_child->GetPadding());

    // TODO(catmull): is this really useful? min_size_ should be empty
    if (first_padding > 0) {
      StreamDim(min_size)  = -first_padding;
      StreamDim(max_size_) = StreamDim(min_size);
    }

    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
      const Layout* const child = entity_->ChildAt(i)->GetLayout();

      if ((child == NULL) || !child->IsInLayout())
        continue;

      const Size child_min = child->GetMinimumSize();
      const Spacing child_pad = child->GetPadding();

      StreamDim(min_size) += StreamDim(child_min);
      if ((prev_padding >= 0) && (StreamBefore(child_pad) >= 0))
        StreamDim(min_size) += std::max(prev_padding, StreamBefore(child_pad));

      CrossDim(min_size) = std::max(
          CrossDim(min_size),
          CrossDim(child_min) +
              std::max<int32_t>(0, CrossBefore(margins)) +
              std::max<int32_t>(0, CrossAfter(margins)));

      const Size child_max = child->GetMaximumSize();

      if (((h_size_ == kSizeDefault) &&
           (child->GetHSizeOption() == kSizeFill)) ||
          (child_max.width == kSizeFill))
        max_size_.width = kSizeFill;
      if (((v_size_ == kSizeDefault) &&
           (child->GetVSizeOption() == kSizeFill)) ||
          (child_max.height == kSizeFill))
        max_size_.height = kSizeFill;

      if (StreamDim(max_size_) != kSizeFill) {
        StreamDim(max_size_) += StreamDim(child_max);
        if ((prev_padding >= 0) && (StreamBefore(child_pad) >= 0))
          StreamDim(max_size_) +=
              std::max(prev_padding, StreamBefore(child_pad));
      }
      if (CrossDim(max_size_) != kSizeFill) {
        CrossDim(max_size_) = std::max(
            CrossDim(max_size_),
            CrossDim(child_max) +
              std::max<int32_t>(0, CrossBefore(margins)) +
              std::max<int32_t>(0, CrossAfter(margins)));
      }

      prev_padding = StreamAfter(child->GetPadding());
    }
  }
  max_size_.width =  std::min(max_size_.width,  min_size.width);
  max_size_.height = std::min(max_size_.height, min_size.height);
  cached_min_size_ = min_size;
  return min_size;
}

void LayoutContainer::SetSizeImp(const Size &size) {
  if (entity_ != NULL)
    entity_->SetNativeProperty(kPropSize, size);
}

void LayoutContainer::SetLocationImp(const Location &loc) {
  if (entity_ != NULL)
    entity_->SetNativeProperty(kPropLocation, loc);
}

void LayoutContainer::ResizeToMinimum() {
  cached_min_size_ = Size();

  for (uint32_t i = 0; i < kMaxLayoutIterations; ++i) {
    const Size min = GetMinimumSize();

    if (min == GetSize())
      break;
    SetSize(min);
    if (min != GetSize())
      cached_min_size_ = Size();
  }
}

Size BorderedContainer::CalculateMinimumSize() const {
  if (cached_min_size_ != Size())
    return cached_min_size_;

  cached_min_size_ = LayoutContainer::CalculateMinimumSize();

  if (entity_->ChildrenCount() != 0) {
    const Spacing margins = GetMargins();

    StreamDim(cached_min_size_) += StreamBefore(margins) + StreamAfter(margins);
    if (StreamDim(max_size_) != kSizeFill)
      StreamDim(max_size_) += StreamBefore(margins) + StreamAfter(margins);
  }
  return cached_min_size_;
}

void Group::ChildAdded(Entity *child) {
  if (entity_->ChildrenCount() == 0) {
    const Value padding = child->GetProperty(kPropPadding);

    if (padding.IsValid())
      min_padding_ = padding.Coerce<Spacing>();
  }
  LayoutContainer::ChildAdded(child);
}

void Group::SetSize(const Size &new_size) {
  LayoutContainer::SetSize(new_size);
  CalculatePadding();
}

void Group::CalculatePadding() {
  // Recalculate the container's padding, which is determined by the children's
  // padding extending outside the container's bounds.
  if (entity_->ChildrenCount() != 0) {
    Spacing new_min_padding;
    Layout *front = NULL, *back = NULL, *child;
    uint32_t i;

    for (i = 0; i < entity_->ChildrenCount(); ++i) {
      child = entity_->ChildAt(i)->GetLayout();
      if ((child != NULL) && child->IsInLayout()) {
        back = child;
        if (front == NULL)
          front = child;
      }
    }
    if (back == NULL) {  // All children are hidden
      min_padding_ = Spacing();
      return;
    }

    StreamBefore(new_min_padding) = StreamBefore(front->GetPadding());
    StreamAfter(new_min_padding)  = StreamAfter(back->GetPadding());

    int32_t &before = CrossBefore(new_min_padding);
    int32_t &after = CrossAfter(new_min_padding);

    for (i = 0; i < entity_->ChildrenCount(); ++i) {
      child = entity_->ChildAt(i)->GetLayout();
      if ((child == NULL) || !child->IsInLayout())
        continue;

      const Spacing pad  = child->GetPadding();
      const Location loc = child->GetLocation();
      const Size size    = child->GetSize();

      before = std::max(before, CrossBefore(pad) - CrossLoc(loc));
      after  = std::max(after, CrossAfter(pad) -
           (CrossDim(size_) - (CrossLoc(loc) + CrossDim(size))));
    }

    if (new_min_padding != min_padding_) {
      Layout *parent = GetLayoutParent();

      min_padding_ = new_min_padding;
      if (parent != NULL)
        parent->InvalidateLayout();
    }
  }
}

bool Group::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropLocation) == 0) {
    SetLocation(value.Coerce<Location>());
    return true;
  }
  if (strcmp(name, kPropValue) == 0) {
    if (entity_->ChildrenCount() > 0)
      return entity_->ChildAt(0)->SetProperty(kPropValue, value);
    else
      return true;
  }
  if (strcmp(name, kPropVisible) == 0) {
    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i)
      entity_->ChildAt(i)->SetProperty(kPropVisible, value);
  }
  return LayoutContainer::SetProperty(name, value);
}

Value Group::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropLocation) == 0)
    return GetLocation();
  if (strcmp(name, kPropSize) == 0)
    return GetSize();
  if (strcmp(name, kPropValue) == 0) {
    if (entity_->ChildrenCount() > 0)
      return entity_->ChildAt(0)->GetProperty(kPropValue);
    else
      return Value();
  }
  return LayoutContainer::GetProperty(name);
}

void Group::ChildValueChanged(Entity *child) {
  DASSERT(entity_->ChildrenCount() > 0);
  if ((child == entity_->ChildAt(0)) && (entity_->GetParent() != NULL))
    entity_->GetParent()->ChildValueChanged(entity_);
}

void Multipanel::Finalize() {
  ShowPanel(0);
}

void Multipanel::CalculatePadding() {
  Spacing new_min_padding;

  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
    Entity* const child = entity_->ChildAt(i);

    if (child->GetLayout() == NULL)
      continue;

    const Spacing child_pad = child->GetLayout()->GetPadding();
    const Size child_size = child->GetLayout()->GetSize();

    if (child_pad.left > new_min_padding.left)
      new_min_padding.left = child_pad.left;
    if (child_pad.top > new_min_padding.top)
      new_min_padding.top = child_pad.top;

    // The child may be smaller in either direction, so check how far the
    // padding actually extends.
    if ((child_size.width + child_pad.right) >
        (size_.width + new_min_padding.right))
      new_min_padding.right = child_size.width + child_pad.right - size_.width;
    if ((child_size.height + child_pad.bottom) >
        (size_.height + new_min_padding.bottom))
      new_min_padding.bottom =
          child_size.height + child_pad.bottom - size_.height;
  }

  if (new_min_padding != min_padding_) {
    Layout *parent = GetLayoutParent();

    min_padding_ = new_min_padding;
    if (parent != NULL)
      parent->InvalidateLayout();
  }
}

bool Multipanel::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropValue) == 0) {
    ShowPanel(value.Coerce<uint32_t>());
    return true;
  }
  if (strcmp(name, kPropVisible) == 0) {
    const bool visible = value.Coerce<bool>();

    if (visible)
      ShowPanel(value_);
    else
      Group::SetProperty(name, value);
    return true;
  }
  return Group::SetProperty(name, value);
}

Value Multipanel::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropValue) == 0)
    return value_;
  return Group::GetProperty(name);
}

void Multipanel::ArrangeObjects(const Size &new_size, uint32_t extra) {
  // Extra parens so this isn't parsed as a function
  const Value location_value((Location()));

  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i)
    entity_->ChildAt(i)->SetProperty(kPropLocation, location_value);
}

Size Multipanel::CalculateMinimumSize() const {
  Size min;

  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
    Value min_value = entity_->ChildAt(i)->GetProperty(kPropMinimumSize);

    if (!min_value.IsValid())
      continue;

    Size child_min = min_value.Coerce<Size>();

    if (child_min.width > min.width)
      min.width = child_min.width;
    if (child_min.height > min.height)
      min.height = child_min.height;
  }
  return min;
}

void Multipanel::ShowPanel(uint32_t index) {
  value_ = index;
  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i)
    entity_->ChildAt(i)->SetProperty(kPropVisible, i == value_);
}

Size Spacer::CalculateMinimumSize() const {
  const PlatformMetrics &metrics = GetPlatformMetrics();
  Size result;

  if (h_size_ == kSizeExplicit)
    result.width = explicit_size_.CalculateWidth(metrics);
  if (v_size_ == kSizeExplicit)
    result.height = explicit_size_.CalculateHeight(metrics);

  return result;
}

}  // namespace Diadem
