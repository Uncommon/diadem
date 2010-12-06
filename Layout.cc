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
    kPropAlign        = "align",
    kPropVisible      = "visible",
    kPropInLayout     = "inLayout",
    kPropDirection    = "direction",
    kPropAmount       = "amount",
    kPropPadding      = "padding",
    kPropMargins      = "margins",
    kPropBaseline     = "baseline";

Bool ParseSizeOption(const char *c, SizeOption *size) {
  const char* strings[3] = {
      "default", "fit", "fill" };
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

void ParseWidth(const char *value, ExplicitSize *size) {
  DASSERT(size != NULL);
  if (strcmp(value, "indent") == 0) {
    size->width_ = 1;
    size->widthUnits_ = kUnitIndent;
  } else {
    const char *letters = FirstLetter(value);

    if ((letters != NULL) && (strcmp(letters, "em") == 0))
      size->widthUnits_ = kUnitEms;
    else
      size->widthUnits_ = kUnitPixels;
    size->width_ = strtof(value, NULL);
  }
}

void ParseHeight(const char *value, ExplicitSize *size) {
  DASSERT(size != NULL);
  const char *letters = FirstLetter(value);

  if ((letters != NULL) && (strcmp(letters, "li") == 0))
    size->heightUnits_ = kUnitLines;
  else
    size->heightUnits_ = kUnitPixels;
  size->height_ = strtof(value, NULL);
}

AlignOption ReverseAlignment(AlignOption a) {
  switch (a) {
    case kAlignStart:  return kAlignEnd;
    case kAlignCenter: return kAlignCenter;
    case kAlignEnd:    return kAlignStart;
  }
  return kAlignStart;
}

void Layout::InitializeProperties(const PropertyMap &properties) {
  const Array<String> keys = properties.AllKeys();

  for (int32_t i = 0; i < keys.size(); ++i)
    SetProperty(keys[i], properties[keys[i]]);
}

LayoutContainer* Layout::GetLayoutParent() {
  if ((entity_ == NULL) || (entity_->GetParent() == NULL))
    return NULL;
  return dynamic_cast<LayoutContainer*>(entity_->GetParent()->GetLayout());
}

const LayoutContainer* Layout::GetLayoutParent() const {
  if ((entity_ == NULL) || (entity_->GetParent() == NULL))
    return NULL;
  return dynamic_cast<const LayoutContainer*>(
      entity_->GetParent()->GetLayout());
}

Bool Layout::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropInLayout) == 0) {
    inLayout_ = value.Coerce<Bool>();
    return true;
  }
  if (strcmp(name, kPropWidthOption) == 0) {
    const String widthString = value.Coerce<String>();

    if (!ParseSizeOption(widthString, &hSize_)) {
      ParseWidth(widthString, &explicitSize_);
      hSize_ = kSizeExplicit;
    }
    return true;
  }
  if (strcmp(name, kPropHeightOption) == 0) {
    const String heightString = value.Coerce<String>();

    if (!ParseSizeOption(heightString, &vSize_)) {
      ParseHeight(heightString, &explicitSize_);
      vSize_ = kSizeExplicit;
    }
    return true;
  }
  if (strcmp(name, kPropAlign) == 0) {
    const String alignString = value.Coerce<String>();

    if (alignString == "start")
      align_ = kAlignStart;
    else if (alignString == "center")
      align_ = kAlignCenter;
    else if (alignString == "end")
      align_ = kAlignEnd;
    return true;
  }
  return false;
}

Value Layout::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropInLayout) == 0) {
    return Value(inLayout_);
  }
  if (strcmp(name, kPropWidthOption) == 0) {
    return Value(static_cast<int>(hSize_));
  }
  return Value();
}

Location Layout::GetViewLocation() const {
  const Location loc = GetLocation();
  const LayoutContainer* const layoutParent = GetLayoutParent();

  if (layoutParent == NULL)
    return loc;
  else
    return loc + layoutParent->GetViewLocation();
}

void Layout::ParentLocationChanged(const Location &offset) {
  if (offset != Location()) {
    const Value oldLocation = entity_->GetProperty(kPropLocation);

    if (oldLocation.IsValid())
      entity_->SetProperty(
          kPropLocation, oldLocation.Coerce<Location>() + offset);
  }
}

Size Layout::CalculateMinimumSize() const {
  return entity_->GetProperty(kPropMinimumSize).Coerce<Size>();
}

Size Layout::EnforceExplicitSize(const Size &size) const {
  if ((hSize_ != kSizeExplicit) && (vSize_ != kSizeExplicit))
    return size;

  Size result = size;
  const PlatformMetrics &metrics = GetPlatformMetrics();

  if (hSize_ == kSizeExplicit) {
    uint32_t hMultiplier = 1;

    switch (explicitSize_.widthUnits_) {
      case kUnitEms:
        hMultiplier = metrics.emSize;
        break;
      case kUnitIndent:
        hMultiplier = metrics.indentSize;
        break;
      default:
        break;
    }
    result.width =
        std::max<long>(size.width, explicitSize_.width_ * hMultiplier);
  }
  if (vSize_ == kSizeExplicit) {
    uint32_t vMultiplier = 1;

    switch (explicitSize_.widthUnits_) {
      case kUnitEms:
        vMultiplier = metrics.emSize;
        break;
      case kUnitIndent:
        vMultiplier = metrics.indentSize;
        break;
      case kUnitLines:
        vMultiplier = metrics.lineHeight;
      default:
        break;
    }
    result.height =
        std::max<long>(size.height, explicitSize_.height_ * vMultiplier);
  }
  return result;
}

void Layout::SetInLayout(Bool inLayout) {
  inLayout_ = inLayout;
  SetVisible(inLayout);
}

const PlatformMetrics& Layout::GetPlatformMetrics() const {
  for (const Entity *e = entity_; e != NULL; e = e->GetParent()) {
    const Native *native = e->GetNative();

    if (native != NULL)
      return native->GetPlatformMetrics();
  }

  static PlatformMetrics noMetrics = {};

  DASSERT(false);
  return noMetrics;
}

Bool LayoutContainer::SetProperty(const char *name, const Value &value) {
  if (strcmp(name, kPropDirection) == 0) {
    if (value.IsValueType<String>()) {
      const String directionString = value.Coerce<String>();

      if (directionString == "row")
        direction_ = kLayoutRow;
      else if (directionString == "column")
        direction_ = kLayoutColumn;
      return true;
    } else {
      direction_ = value.Coerce<LayoutDirection>();
      return true;
    }
  }
  if (strcmp(name, kPropVisible) == 0) {
    visible_ = value.Coerce<Bool>();
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
  const Value nativeMargins = entity_->GetNativeProperty(kPropMargins);

  if (nativeMargins.IsValid())
    return nativeMargins.Coerce<Spacing>();
  return Spacing();
}

void LayoutContainer::InvalidateLayout() {
  cachedMinSize_ = Size();
  layoutValid_ = false;

  LayoutContainer *parent = GetLayoutParent();

  if (parent != NULL)
    parent->InvalidateLayout();
}

void LayoutContainer::SetSize(const Size &s) {
  if (layoutValid_ && (s == GetSize()))
    return;

  Size newSize;
  long extra;

  SetObjectSizes(s, &newSize, &extra);
  ArrangeObjects(newSize, extra);
}

void LayoutContainer::SetLocation(const Location &loc) {
  const Location current = GetLocation();
  const Location offset(loc.x-current.x, loc.y-current.y);

  SetLocationImp(loc);
  for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
    Layout *layoutChild = entity_->ChildAt(i)->GetLayout();

    if (layoutChild != NULL)
      layoutChild->ParentLocationChanged(offset);
  }
}

void LayoutContainer::ParentLocationChanged(const Location &offset) {
  if (offset != Location())
    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
      Layout *layoutChild = entity_->ChildAt(i)->GetLayout();

      if (layoutChild != NULL)
        layoutChild->ParentLocationChanged(offset);
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

void LayoutContainer::SetObjectSizes(
    const Size &s, Size *newSize, long *extra) {
  const Spacing margins = GetMargins();
  const unsigned int fillCount = FillChildCount();

  layoutValid_ = false;
  for (int i = 0; !layoutValid_ && (i < 3); ++i) {  // limit the recalculations
    layoutValid_ = true;
    if (cachedMinSize_ == Size())
      cachedMinSize_ = GetMinimumSize();

    *newSize = Size(
        std::max(s.width, cachedMinSize_.width),
        std::max(s.height, cachedMinSize_.height));
    *extra = ExtraSpace(StreamDim(*newSize));

    SetSizeImp(*newSize);

    const long fill = (fillCount == 0) ? 0 : (*extra / fillCount);
    long remainder  = (fillCount == 0) ? 0 : (*extra % fillCount);

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
        CrossDim(size) = CrossDim(*newSize) -
            (CrossBefore(margins)+CrossAfter(margins));
      else
        CrossDim(size) = CrossDim(min);

      if (fillCount == 0) {
        StreamDim(size) = StreamDim(min);
      } else {  // Distribute the extra space among the fill objects
        if ((StreamSizeOption(*child) == kSizeFill) ||
            (StreamDim(max) == kSizeFill)) {
          long itemFill = fill + StreamDim(min);
          if (remainder > 0) {
            ++itemFill;
            --remainder;
          }
          StreamDim(size) = itemFill;
        } else {
          StreamDim(size) = StreamDim(min);
        }
      }
      child->SetSize(size);
    }
    if (!layoutValid_) {
      cachedMinSize_ = Size();
      CalculateMinimumSize();
    }
  }
  if (fillCount != 0)
    *extra = 0;
}

void LayoutContainer::ArrangeObjects(const Size &newSize, long extra) {
  if (entity_->ChildrenCount() == 0)
    return;

  const Spacing margins = GetMargins();
  long prevPad = StreamBefore(margins);
  long lastEdge = 0;
  const Bool reverseRow = (direction_ == kLayoutRow) && IsRTL();
  const Bool reverseCol = (direction_ == kLayoutColumn) && IsRTL();

  switch (reverseRow ? ReverseAlignment(streamAlign_) : streamAlign_) {
    case kAlignStart:
      break;
    case kAlignCenter:
      lastEdge += extra / 2;
      break;
    case kAlignEnd:
      lastEdge += extra;
      break;
  }

  long crossExtra;
  Bool first = true;
  const uint32_t end = reverseRow ? UINT32_MAX : entity_->ChildrenCount();
  const int inc = reverseRow ? -1 : 1;

  for (uint32_t i = reverseRow ? entity_->ChildrenCount()-1 : 0;
       i != end; i += inc) {
    Layout *child = entity_->ChildAt(i)->GetLayout();

    if ((child == NULL) || !child->IsInLayout())
      continue;

    const Spacing pPadding = child->GetPadding();
    const Size childSize(child->GetSize());
    Location loc;
    long before = 0;

    if (first)
      before = StreamBefore(margins);
    else if ((prevPad >= 0) && (StreamBefore(pPadding) >= 0))
      before = std::max(StreamBefore(pPadding), prevPad);
    StreamLoc(loc) = before + lastEdge;
    CrossLoc(loc)  = CrossBefore(margins);
    const AlignOption childAlign = reverseCol ?
        ReverseAlignment(child->GetAlignment()) : child->GetAlignment();

    switch (childAlign) {
      case kAlignStart:
        break;
      case kAlignCenter:
      case kAlignEnd:
        crossExtra = CrossDim(newSize)-CrossDim(childSize) -
            (CrossBefore(margins) + CrossAfter(margins));
        if (childAlign == kAlignEnd)
          CrossLoc(loc) += crossExtra;
        else
          CrossLoc(loc) += crossExtra / 2;
        break;
    }
    child->SetLocation(loc);
    lastEdge = StreamLoc(child->GetLocation()) + StreamDim(child->GetSize());
    prevPad = StreamAfter(pPadding);
    first = false;
  }
  AlignBaselines();
}

void LayoutContainer::AlignBaselines() {
  if (direction_ != kLayoutRow)
    return;

  Layout *bestItems[3] = { NULL, NULL, NULL };
  long baselines[3] = { 0, 0, GetSize().height };
  long centerHeight = 0;
  AlignOption a;
  uint32 i;
  Layout *child = NULL;

  for (i = 0; i < entity_->ChildrenCount(); ++i) {
    child = entity_->ChildAt(i)->GetLayout();
    if ((child == NULL) || !child->IsInLayout())
      continue;

    const long baseline = child->GetBaseline();

    switch (child->GetAlignment()) {
      case kAlignStart:
        if (baseline > baselines[0]) {
          baselines[0] = baseline;
          bestItems[0] = child;
        }
        break;
      case kAlignCenter:
        if (child->GetSize().height > centerHeight) {
          centerHeight = child->GetSize().height;
          baselines[1] = baseline;
          bestItems[1] = child;
        }
        break;
      case kAlignEnd:
        if (child->GetBaseline() < baselines[2]) {
          baselines[2] = baseline;
          bestItems[2] = child;
        }
        break;
    }
  }
  for (i = 0; i < entity_->ChildrenCount(); ++i) {
    child = entity_->ChildAt(i)->GetLayout();
    if ((child == NULL) || !child->IsInLayout())
      continue;

    a = child->GetAlignment();
    if (child == bestItems[a])
      continue;

    const long baseline = child->GetBaseline();

    if (baseline != 0) {
      Location loc(child->GetLocation());

      loc.y += baselines[a] - baseline;
      child->SetLocation(loc);
    }
  }
}

Size LayoutContainer::CalculateMinimumSize() const {
  if (cachedMinSize_ != Size())
    return cachedMinSize_;

  Size minSize;

  maxSize_ = Size();
  if (entity_->ChildrenCount() != 0) {
    const Spacing margins = GetMargins();
    long prevPadding = 0;
    const Layout* const firstChild =
        entity_->ChildAt(0)->GetLayout();

    const long firstPadding = StreamBefore(firstChild->GetPadding());

    // TODO(catmull): is this really useful? minSize should be empty
    if (firstPadding > 0) {
      StreamDim(minSize)  = -firstPadding;
      StreamDim(maxSize_) = StreamDim(minSize);
    }

    for (uint32_t i = 0; i < entity_->ChildrenCount(); ++i) {
      const Layout* const child =
          entity_->ChildAt(i)->GetLayout();

      if ((child == NULL) || !child->IsInLayout())
        continue;

      const Size childMin = child->GetMinimumSize();
      const Spacing childPad = child->GetPadding();

      StreamDim(minSize) += StreamDim(childMin);
      if ((prevPadding >= 0) && (StreamBefore(childPad) >= 0))
        StreamDim(minSize) += std::max(prevPadding, StreamBefore(childPad));

      CrossDim(minSize) = std::max(
          CrossDim(minSize),
          CrossDim(childMin) +
              std::max<long>(0, CrossBefore(margins)) +
              std::max<long>(0, CrossAfter(margins)));

      const Size childMax = child->GetMaximumSize();

      if (((hSize_ == kSizeDefault) &&
           (child->GetHSizeOption() == kSizeFill)) ||
          (childMax.width == kSizeFill))
        maxSize_.width = kSizeFill;
      if (((vSize_ == kSizeDefault) &&
           (child->GetVSizeOption() == kSizeFill)) ||
          (childMax.height == kSizeFill))
        maxSize_.height = kSizeFill;

      if (StreamDim(maxSize_) != kSizeFill) {
        StreamDim(maxSize_) += StreamDim(childMax);
        if ((prevPadding >= 0) && (StreamBefore(childPad) >= 0))
          StreamDim(maxSize_) += std::max(prevPadding, StreamBefore(childPad));
      }
      if (CrossDim(maxSize_) != kSizeFill) {
        CrossDim(maxSize_) = std::max(
            CrossDim(maxSize_),
            CrossDim(childMax) +
              std::max<long>(0, CrossBefore(margins)) +
              std::max<long>(0, CrossAfter(margins)));
      }

      prevPadding = StreamAfter(child->GetPadding());
    }
  }
  maxSize_.width =  std::min(maxSize_.width,  minSize.width);
  maxSize_.height = std::min(maxSize_.height, minSize.height);
  cachedMinSize_ = minSize;
  return minSize;
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
  cachedMinSize_ = Size();
  layoutValid_ = false;

  for (int i = 0; i < 3; ++i) {
    const Size min = GetMinimumSize();

    if (min == GetSize())
      break;
    SetSize(min);
  }
}

Size BorderedContainer::CalculateMinimumSize() const {
  if (cachedMinSize_ != Size())
    return cachedMinSize_;

  cachedMinSize_ = LayoutContainer::CalculateMinimumSize();

  if (entity_->ChildrenCount() != 0) {
    const Spacing margins = GetMargins();

    StreamDim(cachedMinSize_) += StreamBefore(margins) + StreamAfter(margins);
    if (StreamDim(maxSize_) != kSizeFill)
      StreamDim(maxSize_) += StreamBefore(margins) + StreamAfter(margins);
  }
  return cachedMinSize_;
}

void Group::ChildAdded(Entity *child) {
  if (entity_->ChildrenCount() == 0) {
    const Value padding = child->GetProperty(kPropPadding);

    if (padding.IsValid())
      minPadding_ = padding.Coerce<Spacing>();
  }
  LayoutContainer::ChildAdded(child);
}

void Group::SetSize(const Size &newSize) {
  LayoutContainer::SetSize(newSize);

  // Recalculate the container's padding, which is determined by the children's
  // padding extending outside the container's bounds.
  if (entity_->ChildrenCount() != 0) {
    Spacing newMinPadding;
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
      minPadding_ = Spacing();
      return;
    }

    StreamBefore(newMinPadding) = StreamBefore(front->GetPadding());
    StreamAfter(newMinPadding)  = StreamAfter(back->GetPadding());

    long &before = CrossBefore(newMinPadding);
    long &after = CrossAfter(newMinPadding);

    for (i = 0; i < entity_->ChildrenCount(); ++i) {
      child = entity_->ChildAt(i)->GetLayout();
      if ((child == NULL) || !child->IsInLayout())
        continue;

      const Spacing pad  = child->GetPadding();
      const Location loc = child->GetLocation();
      const Size size    = child->GetSize();

      before = std::max(before, CrossBefore(pad) - CrossLoc(loc));
      after  = std::max(before, CrossAfter(pad) -
           (CrossDim(newSize) - (CrossLoc(loc) + CrossDim(size))));
    }

    if (newMinPadding != minPadding_) {
      LayoutContainer *parent = GetLayoutParent();

      minPadding_ = newMinPadding;
      if (parent != NULL)
        parent->InvalidateLayout();
    }
  }
}

Bool Group::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropLocation) == 0) {
    SetLocation(value.Coerce<Location>());
    return true;
  }
  return LayoutContainer::SetProperty(name, value);
}

Value Group::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropLocation) == 0)
    return GetLocation();
  return LayoutContainer::GetProperty(name);
}

Size Spacer::CalculateMinimumSize() const {
  const PlatformMetrics &metrics = GetPlatformMetrics();
  Size result;

  if (hSize_ == kSizeExplicit)
    result.width = explicitSize_.CalculateWidth(metrics);
  if (vSize_ == kSizeExplicit)
    result.height = explicitSize_.CalculateHeight(metrics);

  return result;
}

}  // namespace Diadem
