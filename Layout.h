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
  kSizeFill = -1,   // Fill any extra space in the container.
  kSizeDefault,     // Depends on the type, usually fit or fill.
  kSizeFit,         // Just big enough to fit its contents.
  kSizeExplicit,    // Given in pixels or platform-dependent units.
};

enum AlignOption {
  kAlignStart,    // Top for rows, left for columns.
  kAlignCenter,
  kAlignEnd       // Bottom for rows, right for columns.
};

extern const PropertyName
    kPropSize, kPropMinimumSize, kPropMaxWidth, kPropMaxHeight,
    kPropWidthOption, kPropHeightOption, kPropDirection,
    kPropWidthName, kPropHeightName,
    kPropLocation, kPropAlign, kPropVisible, kPropInLayout,
    kPropPadding, kPropMargins, kPropBaseline;

extern const TypeName kTypeNameGroup, kTypeNameSpacer;

extern const StringConstant kAlignNameStart, kAlignNameCenter, kAlignNameEnd;

extern const StringConstant kDirectionNameRow, kDirectionNameColumn;

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

  // Notification that a child has been added, in case layout adjustments
  // need to be made.
  virtual void ChildAdded(Entity *child) {}

  // These are convenience methods to get the layout object for the parent
  // entity.
  Layout* GetLayoutParent();
  const Layout* GetLayoutParent() const;

  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  virtual void Finalize() { ResizeToMinimum(); }

  // Layout is done repeatedly until nobody calls InvalidateLayout.
  virtual void InvalidateLayout();

  // Returns the layout direction of the object, or the parent container.
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

  // Padding is the minimum distance around an object.
  virtual Spacing GetPadding() const
    { return entity_->GetNativeProperty(kPropPadding).Coerce<Spacing>(); }
  // Baseline is the distance from the top to the baseline of the text.
  virtual long GetBaseline() const
    { return entity_->GetProperty(kPropBaseline).Coerce<int32_t>(); }

  AlignOption GetAlignment() const { return align_; }

  virtual Size GetMinimumSize() const
    { return EnforceExplicitSize(CalculateMinimumSize()); }
  virtual Size GetMaximumSize() const
    { return GetMinimumSize(); }
  virtual void ResizeToMinimum()
    { SetSize(GetMinimumSize()); }

  // Visibility is simply whether the object is displayed, though it still
  // may take up space in the layout.
  void SetVisible(bool visible)
    { entity_->SetNativeProperty(kPropVisible, visible); }
  bool IsVisible() const
    { return entity_->GetNativeProperty(kPropVisible).Coerce<bool>(); }

  // "Not in layout" means it takes up no space in the layout. It also implies
  // not visible.
  void SetInLayout(bool in_layout);
  bool IsInLayout() const { return in_layout_; }

  const String& GetWidthName() const  { return width_name_; }
  const String& GetHeightName() const { return height_name_; }
  void SetWidthName(const String &name)  { width_name_ = name; }
  void SetHeightName(const String &name) { height_name_ = name; }

  // Notification that the parent's location has changed, so this child should
  // adjust itself accordingly.
  virtual void ParentLocationChanged(const Location &offset);

  SizeOption GetHSizeOption() const { return h_size_; }
  SizeOption GetVSizeOption() const { return v_size_; }
  void SetHSizeOption(SizeOption h_size) { h_size_ = h_size; }
  void SetVSizeOption(SizeOption v_size) { v_size_ = v_size; }
  const ExplicitSize& GetExplicitSize() { return explicit_size_; }

  const PlatformMetrics& GetPlatformMetrics() const;

 protected:
  bool in_layout_;
  SizeOption h_size_, v_size_;
  ExplicitSize explicit_size_;
  String width_name_, height_name_;
  AlignOption align_;

  // For each dimension, returns the greater of the given size and any
  // explicit size specified in the object.
  Size EnforceExplicitSize(const Size &size) const;
  // Returns the smallest size allowed by the object.
  virtual Size CalculateMinimumSize() const;

  enum Dimension {
    kDimensionWidth,
    kDimensionHeight
  };

  // Width and height names:
  // When an object has its width or height name set, it will have the same
  // width/height as other objects with the same width/height name. The largest
  // object determines the size of the others.

  // Find the width to be used for the object's width name.
  uint32_t FindWidthForName() const;
  // Find the height to be used for the object's height name.
  uint32_t FindHeightForName() const;
  // Called on the root layout object. Recursively finds the largest dimension
  // (width or height) of an object with the given width/height name.
  uint32_t FindDimensionForName(Dimension dimension, const String &name) const;
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
      : direction_(kLayoutRow), visible_(true),
        stream_align_(kAlignStart), cross_align_(kAlignStart) {}
  virtual ~LayoutContainer() {}

  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  virtual LayoutDirection GetDirection() const { return direction_; }

  // Sets the object's size and recalculates the layout of its children.
  virtual void SetSize(const Size &size);
  // Moves the object and its children to the given location.
  virtual void SetLocation(const Location &loc);
  virtual void ResizeToMinimum();
  virtual Size CalculateMinimumSize() const;
  // Returns the space between this object's edges and the edges of any
  // children. Margins and padding will overlap.
  virtual Spacing GetMargins() const;
  virtual long GetBaseline() const;

  // Signals the layout code to continue with another iteration because
  // something affecting layout has changed.
  virtual void InvalidateLayout();

 protected:
  LayoutDirection direction_;
  bool visible_;
  AlignOption stream_align_, cross_align_;
  mutable Size cached_min_size_;
  mutable Size max_size_;

  // Layout is done in two main phases: setting sizes, and setting locations.
  void SetObjectSizes(const Size &s, Size *new_size, long *extra);
  void ArrangeObjects(const Size &new_size, long extra);

  // Pass size changes to the native implementation.
  virtual void SetSizeImp(const Size &size);
  // Pass location changes to the native implementation.
  virtual void SetLocationImp(const Location &loc);
  // Pass location changes to the children.
  virtual void ParentLocationChanged(const Location &offset);

  // Returns the extra space available to child objects that may want to expand
  // to fill it.
  virtual long ExtraSpace(long new_size)
    { return new_size - StreamDim(CalculateMinimumSize()); }

  // Adjusts children vertically so their baselines line up.
  void AlignBaselines();
  // Returns the number of children set to fill in the layout direction.
  uint32_t FillChildCount() const;

  // Return true if the application is running in an RTL language, indicating
  // to ArrangeObjects that rows should be laid out in reverse order.
  bool IsRTL() const { return false; }  // TODO(catmull): implement

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
// (row vs column) of its parent. A group's padding is determined by how much
// the children's padding extends past the group's borders.
class Group : public LayoutContainer {
 public:
  Group() {}

  virtual String GetTypeName() const { return kTypeNameGroup; }

  // Padding may need to be recalculated when children are added.
  virtual void ChildAdded(Entity *child);

  Spacing GetPadding() const { return min_padding_; }

  // When the group's size changes, the children may move, which could change
  // the group's padding.
  virtual void SetSize(const Size &size);
  virtual Size GetSize() const { return size_; }
  virtual Location GetLocation() const { return location_; }

  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  // The group's value is the value of the first child, so changes may need
  // to be propagated.
  virtual void ChildValueChanged(Entity *child);

 protected:
  Size size_;
  Location location_;
  Spacing min_padding_;

  void CalculatePadding();

  void SetSizeImp(const Size &size)        { size_ = size; }
  void SetLocationImp(const Location &loc) { location_ = loc; }
};

// A layout entity that overrides the default space between entities.
// The negative padding is treated as a special value by ArrangeObjects.
class Spacer : public SelfMeasured {
 public:
  Spacer() { padding_ = Spacing(-1, -1, -1, -1); }

  virtual String GetTypeName() const { return kTypeNameSpacer; }

 protected:
  virtual Size CalculateMinimumSize() const;
};

// Finds the first letter (a-z) in a string.
const char *FirstLetter(const char *s);

}  // namespace Diadem

#endif  // DIADEM_LAYOUT_H_
