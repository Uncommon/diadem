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

#ifndef DIADEM_LAYOUT_H_
#define DIADEM_LAYOUT_H_

#include "Diadem/Entity.h"
#include "Diadem/Metrics.h"
#include "Diadem/Value.h"

namespace Diadem {

class LayoutContainer;

enum SizeOption {
  kSizeFill = -1,
  kSizeDefault,
  kSizeFit,
  kSizeExplicit
};

enum AlignOption {
  kAlignStart,
  kAlignCenter,
  kAlignEnd
};

extern const PropertyName
    kPropSize, kPropMinimumSize, kPropMaxWidth, kPropMaxHeight,
    kPropWidthOption, kPropHeightOption, kPropDirection,
    kPropLocation, kPropAlign, kPropVisible, kPropInLayout,
    kPropPadding, kPropMargins, kPropBaseline;

// A layout object manages an Entity's place in the dialog layout.
class Layout : public EntityDelegate {
 public:
  enum LayoutDirection { kLayoutRow, kLayoutColumn };

  Layout()
      : in_layout_(true),
        h_size_(kSizeDefault), v_size_(kSizeDefault),
        align_(kAlignStart) {}
  virtual ~Layout() {}

  void InitializeProperties(const PropertyMap &properties);

  virtual void ChildAdded(Entity *child) {}

  // These are convenience methods to get the layout object for the parent
  // entity.
  Layout* GetLayoutParent();
  const Layout* GetLayoutParent() const;

  virtual Bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  virtual void Finalize() { ResizeToMinimum(); }

  // Layout is done repeatedly until nobody calls InvalidateLayout.
  virtual void InvalidateLayout();

  virtual LayoutDirection GetDirection() const;

  virtual void SetSize(const Size &size)
    { entity_->SetNativeProperty(kPropSize, size); }
  virtual Size GetSize() const
    { return entity_->GetNativeProperty(kPropSize).Coerce<Size>(); }

  virtual void SetLocation(const Location &loc)
    { entity_->SetNativeProperty(kPropLocation, loc); }
  virtual Location GetLocation() const
    { return entity_->GetNativeProperty(kPropLocation).Coerce<Location>(); }

  // Get the location relative to the native view. This may be different from
  // GetLocation() if the object is inside a Group.
  virtual Location GetViewLocation() const;

  // Padding is the minimum distance around an object
  virtual Spacing GetPadding() const
    { return entity_->GetNativeProperty(kPropPadding).Coerce<Spacing>(); }
  // Baseline is the distance from the top to the baseline of the text
  virtual long GetBaseline() const
    { return entity_->GetProperty(kPropBaseline).Coerce<int32_t>(); }

  AlignOption GetAlignment() const { return align_; }

  virtual Size GetMinimumSize() const
    { return EnforceExplicitSize(CalculateMinimumSize()); }
  virtual Size GetMaximumSize() const
    { return GetMinimumSize(); }
  virtual void ResizeToMinimum()
    { SetSize(GetMinimumSize()); }

  // Visibility is simply whether the object is displayed
  void SetVisible(Bool visible)
    { entity_->SetNativeProperty(kPropVisible, visible); }
  Bool IsVisible() const
    { return entity_->GetNativeProperty(kPropVisible).Coerce<Bool>(); }

  // "Not in layout" means it takes up no space in the layout. It also implies
  // not visible.
  void SetInLayout(Bool in_layout);
  Bool IsInLayout() const { return in_layout_; }

  virtual void ParentLocationChanged(const Location &offset);

  SizeOption GetHSizeOption() const { return h_size_; }
  SizeOption GetVSizeOption() const { return v_size_; }
  void SetHSizeOption(SizeOption h_size) { h_size_ = h_size; }
  void SetVSizeOption(SizeOption v_size) { v_size_ = v_size; }

  const PlatformMetrics& GetPlatformMetrics() const;

 protected:
  Bool in_layout_;
  SizeOption h_size_, v_size_;
  ExplicitSize explicit_size_;
  AlignOption align_;

  Size EnforceExplicitSize(const Size &size) const;
  virtual Size CalculateMinimumSize() const;
};

// Size, location and padding are stored in the object. Other Layout
// subclasses may use calculated values or retrieve from the Native object.
class SelfMeasured : public Layout {
 public:
  SelfMeasured() {}

  virtual void SetSize(const Size &size) { size_ = size; }
  virtual Size GetSize() const { return size_; }

  virtual void SetLocation(const Location &loc) { location_ = loc;}
  virtual Location GetLocation() const { return location_; }

  virtual void SetPadding(const Spacing &padding) { padding_ = padding; }
  virtual Spacing GetPadding() const { return padding_; }

 protected:
  Size size_;
  Location location_;
  Spacing padding_;
};

// A layout entity that contains others inside it. This is where most of the
// real layout action happens.
class LayoutContainer : public Layout {
 public:
  LayoutContainer()
      : direction_(kLayoutRow), visible_(true), layout_valid_(false),
        stream_align_(kAlignStart), cross_align_(kAlignStart) {}
  virtual ~LayoutContainer() {}

  virtual Bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  virtual LayoutDirection GetDirection() const { return direction_; }

  virtual void SetSize(const Size &size);
  virtual void SetLocation(const Location &loc);
  virtual void ResizeToMinimum();
  virtual Size CalculateMinimumSize() const;
  virtual Spacing GetMargins() const;

  virtual void InvalidateLayout();

 protected:
  LayoutDirection direction_;
  Bool visible_, layout_valid_;
  AlignOption stream_align_, cross_align_;
  mutable Size cached_min_size_;
  mutable Size max_size_;

  // Layout is done in two main phases: setting sizes, and setting locations.
  void SetObjectSizes(const Size &s, Size *new_size, long *extra);
  void ArrangeObjects(const Size &new_size, long extra);

  virtual void SetSizeImp(const Size &size);
  virtual void SetLocationImp(const Location &loc);
  virtual void ParentLocationChanged(const Location &offset);

  // Extra space available to child objects that may want to expand to fill it
  virtual long ExtraSpace(long new_size)
    { return new_size - StreamDim(CalculateMinimumSize()); }

  void AlignBaselines();
  uint32_t FillChildCount() const;

  // Return true if the application is running in an RTL language, indicating
  // to ArrangeObjects that rows should be laid out in reverse order.
  Bool IsRTL() const { return false; }  // TODO(catmull): implement

  // Convenience functions to make layout code more readable.  For rows,
  // "stream" is horizontal and "cross" is vertical.  Opposite for columns.
  long& StreamDim(Size &s) const
    { return (direction_ == kLayoutRow)    ? s.width : s.height; }
  long StreamDim(const Size &s) const
    { return (direction_ == kLayoutRow)    ? s.width : s.height; }
  long& CrossDim(Size &s) const
    { return (direction_ == kLayoutColumn) ? s.width : s.height; }
  long CrossDim(const Size &s) const
    { return (direction_ == kLayoutColumn) ? s.width : s.height; }
  long& StreamLoc(Location &l) const
    { return (direction_ == kLayoutRow)    ? l.x : l.y; }
  long StreamLoc(const Location &l) const
    { return (direction_ == kLayoutRow)    ? l.x : l.y; }
  long& CrossLoc(Location &l) const
    { return (direction_ == kLayoutColumn) ? l.x : l.y; }
  long CrossLoc(const Location &l) const
    { return (direction_ == kLayoutColumn) ? l.x : l.y; }
  long& StreamBefore(Spacing &s) const
    { return (direction_ == kLayoutRow) ? s.left : s.top; }
  long StreamBefore(const Spacing &s) const
    { return (direction_ == kLayoutRow) ? s.left : s.top; }
  long& StreamAfter(Spacing &s) const
    { return (direction_ == kLayoutRow) ? s.right : s.bottom; }
  long StreamAfter(const Spacing &s) const
    { return (direction_ == kLayoutRow) ? s.right : s.bottom; }
  long& CrossBefore(Spacing &s) const
    { return (direction_ == kLayoutColumn) ? s.left : s.top; }
  long CrossBefore(const Spacing &s) const
    { return (direction_ == kLayoutColumn) ? s.left : s.top; }
  long& CrossAfter(Spacing &s) const
    { return (direction_ == kLayoutColumn) ? s.right : s.bottom; }
  long CrossAfter(const Spacing &s) const
    { return (direction_ == kLayoutColumn) ? s.right : s.bottom; }
  SizeOption StreamSizeOption(const Layout &entity) const {
    return (direction_ == kLayoutRow) ?
        entity.GetHSizeOption() : entity.GetVSizeOption();
  }
  SizeOption CrossSizeOption(const Layout &entity) const {
    return (direction_ == kLayoutColumn) ?
        entity.GetHSizeOption() : entity.GetVSizeOption();
  }
};

// A container with inside margins. Children are laid out inside the margins,
// ignoring padding at the edges.
class BorderedContainer : public LayoutContainer {
 public:
  BorderedContainer() { direction_ = kLayoutColumn; }
  virtual ~BorderedContainer() {}

  virtual Size CalculateMinimumSize() const;
};

// Not to be confused with group boxes that have visible borders. A Group is
// simply a layout tool. Normally its layout direction will be the opposite
// (row vs column) of its parent.
class Group : public LayoutContainer {
 public:
  Group() {}

  virtual void ChildAdded(Entity *child);

  Spacing GetPadding() const { return min_padding_; }

  virtual void SetSize(const Size &size);
  virtual Size GetSize() const { return size_; }
  virtual Location GetLocation() const { return location_; }

  virtual Bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

 protected:
  Size size_;
  Location location_;
  Spacing min_padding_;

  void CalculatePadding();

  void SetSizeImp(const Size &size)        { size_ = size; }
  void SetLocationImp(const Location &loc) { location_ = loc; }

  virtual void ParentLocationChanged(const Location &offset) {
    location_ += offset;
    LayoutContainer::ParentLocationChanged(offset);
  }
};

// A layout entity that overrides the default space between entities.
// The negative padding is treated as a special value by ArrangeObjects.
class Spacer : public SelfMeasured {
 public:
  Spacer() { padding_ = Spacing(-1, -1, -1, -1); }

 protected:
  virtual Size CalculateMinimumSize() const;
};

}  // namespace Diadem

#endif  // DIADEM_LAYOUT_H_
