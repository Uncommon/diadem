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

#include "Diadem/NativeCarbon.h"

#include <algorithm>

#include "Diadem/Factory.h"
#include "Diadem/Layout.h"

namespace {

CFStringRef CFStringFromString(const Diadem::String &dstring) {
  return ::CFStringCreateWithCString(
      kCFAllocatorDefault, dstring.Get(), kCFStringEncodingUTF8);

}  // namespace

Diadem::String StringFromCFString(CFStringRef cfString) {
  const CFIndex length = ::CFStringGetBytes(
      cfString, CFRangeMake(0, ::CFStringGetLength(cfString)),
      kCFStringEncodingUTF8, '.', false, NULL, 0, NULL);

  if (length == 0)
    return Diadem::String();

  char *buffer = new char[length+1];

  ::CFStringGetBytes(
      cfString, CFRangeMake(0, ::CFStringGetLength(cfString)),
      kCFStringEncodingUTF8, '.', false,
      reinterpret_cast<UInt8*>(buffer), length, NULL);
  buffer[length] = '\0';
  return Diadem::String(buffer, Diadem::String::kAdoptBuffer);
}

enum RetainAction { kDontRetain, kDoRetain };

template <class T>
class ScopedCFType {
 public:
  ScopedCFType() : ref_(NULL) {}
  ScopedCFType(T ref, RetainAction action)
      : ref_(ref) {
    if (action == kDoRetain)
      ::CFRetain(ref_);
  }
  ~ScopedCFType() {
    if (ref_ != NULL)
      ::CFRelease(ref_);
  }

  void Set(T ref, RetainAction action) {
    if (ref_ != NULL)
      ::CFRelease(ref_);
    ref_ = ref;
    if (action == kDoRetain)
      ::CFRetain(ref_);
  }
  T* RetainedOutPtr() {
    if (ref_ != NULL) {
      ::CFRelease(ref_);
      ref_ = NULL;
    }
    return &ref_;
  }
  operator T() const { return ref_; }

 protected:
  T ref_;
};

}  // namespace

namespace Diadem {

PlatformMetrics Carbon::NativeCarbon::metrics_ = {
    14, 17, 18,
    Spacing(12, 6, 12, 6) };

void Carbon::SetUpFactory(Factory *factory) {
  DASSERT(factory != NULL);
  factory->RegisterNative<Label>("label");
  factory->RegisterNative<Window>("window");
  factory->RegisterNative<Button>("button");
  factory->RegisterNative<Checkbox>("check");
  factory->RegisterNative<EditField>("edit");
  factory->RegisterNative<Separator>("separator");
}

void Carbon::Window::InitializeProperties(const PropertyMap &properties) {
  const Rect defaultBounds = { 40, 0, 50, 50 };
  OSStatus err;

  err = ::CreateNewWindow(
      kMovableModalWindowClass,
      kWindowAsyncDragAttribute |
          kWindowStandardHandlerAttribute |
          kWindowCompositingAttribute,
      &defaultBounds, &windowRef_);
  if ((err == noErr) && (windowRef_ != NULL)) {
    if (properties.Exists(Entity::kPropText)) {
      const String titleString = properties[Entity::kPropText].Coerce<String>();
      ScopedCFType<CFStringRef> cfTitle(
          CFStringFromString(titleString),
          kDontRetain);

      ::SetWindowTitleWithCFString(windowRef_, cfTitle);
    }
  }
}

Carbon::Window::~Window() {
  if (windowRef_ != NULL)
    ::CFRelease(windowRef_);
}

Bool Carbon::Window::SetProperty(PropertyName name, const Value &value) {
  if (windowRef_ == NULL)
    return false;
  if (strcmp(name, Entity::kPropText) == 0) {
    ScopedCFType<CFStringRef> title(
        CFStringFromString(value.Coerce<String>()),
        kDontRetain);
    ::SetWindowTitleWithCFString(windowRef_, title);
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>();
    Rect bounds;

    ::GetWindowBounds(windowRef_, kWindowContentRgn, &bounds);
    bounds.right = bounds.left + size.width;
    bounds.bottom = bounds.top + size.height;
    ::SetWindowBounds(windowRef_, kWindowContentRgn, &bounds);
    // TODO(catmull): reposition?
    return true;
  }
  return false;
}

Value Carbon::Window::GetProperty(PropertyName name) const {
  if (windowRef_ == NULL)
    return false;
  if (strcmp(name, Entity::kPropText) == 0) {
    ScopedCFType<CFStringRef> title;

    ::CopyWindowTitleAsCFString(windowRef_, title.RetainedOutPtr());
    return StringFromCFString(title);
  }
  if (strcmp(name, kPropSize) == 0) {
    Rect bounds;

    ::GetWindowBounds(windowRef_, kWindowContentRgn, &bounds);
    return Size(bounds.right-bounds.left, bounds.bottom-bounds.top);
  }
  if (strcmp(name, kPropMargins) == 0) {
    return Value(Spacing(14, 20, 20, 20));
  }
  return Value();
}

void Carbon::Window::AddChild(Native *child) {
  if (!::HIViewIsValid((HIViewRef)child->GetNativeRef()))
    return;

  HIViewRef contentView = NULL;

  ::HIViewFindByID(
      ::HIViewGetRoot(windowRef_), kHIViewWindowContentID, &contentView);
  if (contentView != NULL)
    ::HIViewAddSubview(contentView, (HIViewRef)child->GetNativeRef());
}

Bool Carbon::Window::ShowModeless() {
  if (windowRef_ == NULL)
    return false;
  ::SelectWindow(windowRef_);
  ::ShowWindow(windowRef_);
  return true;
}

Bool Carbon::Window::Close() {
  if (windowRef_ == NULL)
    return false;
  ::HideWindow(windowRef_);
  return true;
}

Bool Carbon::Window::ShowModal(void *onParent) {
  if (windowRef_ == NULL)
    return false;

  // TODO(catmull): defer to main thread

  WindowPtr parent = ::GetFrontWindowOfClass(kDocumentWindowClass, true);
  WindowPositionMethod windowPosition;

  if (parent == NULL)
    parent = ::GetFrontWindowOfClass(kMovableModalWindowClass, true);
  if (parent != NULL) {
    // We use alert position in both cases because that's actually the
    // preferred location for new windows in the HIG. In the case where it
    // has been marked as an alert and has a parent window, we assume it's
    // strongly associated with that window (i.e., it would be a sheet if
    // not for our modality needs), so we give it window alert position.
    windowPosition = isAlert_ ? kWindowAlertPositionOnParentWindow
                              : kWindowAlertPositionOnParentWindowScreen;
  } else {
    windowPosition = kWindowAlertPositionOnMainScreen;
  }
  ::RepositionWindow(windowRef_, parent, windowPosition);
  ::SetThemeCursor(kThemeArrowCursor);
  ::ShowWindow(windowRef_);
  ::SelectWindow(windowRef_);
  ::RunAppModalLoopForWindow(windowRef_);
  return true;
}

Bool Carbon::Window::EndModal() {
  if (windowRef_ == NULL)
    return false;

  // TODO(catmull): defer to main thread

  // TODO(catmull): maybe check window modality for good measure

  ::QuitAppModalLoopForWindow(windowRef_);
  ::HideWindow(windowRef_);
  return true;
}

Bool Carbon::Window::SetFocus(Entity *newFocus) {
  if ((windowRef_ == NULL) || (newFocus == NULL) ||
      (newFocus->GetNative() == NULL) ||
      (newFocus->GetNative()->GetNativeRef() == NULL))
    return false;
  if (!::IsValidControlHandle((HIViewRef)
      newFocus->GetNative()->GetNativeRef()))
    return false;

  ::SetKeyboardFocus(
      windowRef_,
      (HIViewRef)newFocus->GetNative()->GetNativeRef(),
      kControlFocusNextPart);
  return true;
}

Carbon::Control::~Control() {
  if (viewRef_ != NULL)
    ::CFRelease(viewRef_);
}

Bool Carbon::Control::SetProperty(PropertyName name, const Value &value) {
  if (viewRef_ == NULL)
    return false;
  if (strcmp(name, kPropLocation) == 0) {
    const Location loc = value.Coerce<Location>() + GetViewOffset();

    ::HIViewPlaceInSuperviewAt(viewRef_, loc.x, loc.y);
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>() + GetInset();
    HIRect frame;

    ::HIViewGetFrame(viewRef_, &frame);
    frame.size.width = size.width;
    frame.size.height = size.height;
    ::HIViewSetFrame(viewRef_, &frame);
    return true;
  }
  if (strcmp(name, Entity::kPropText) == 0) {
    ScopedCFType<CFStringRef> cfText(
        CFStringFromString(value.Coerce<String>()),
        kDontRetain);

    return ::HIViewSetText(viewRef_, cfText) == noErr;
  }
  if (strcmp(name, kPropVisible) == 0) {
    ::HIViewSetVisible(viewRef_, value.Coerce<Bool>());
  }
  return false;
}

Value Carbon::Control::GetProperty(PropertyName name) const {
  if (viewRef_ == NULL)
    return Value();
  if (strcmp(name, Entity::kPropText) == 0) {
    ScopedCFType<CFStringRef> cfText(::HIViewCopyText(viewRef_), kDontRetain);

    return StringFromCFString(cfText);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    HIRect bounds;

    ::HIViewGetOptimalBounds(viewRef_, &bounds, NULL);
    return Size(bounds.size.width, bounds.size.height) - GetInset();
  }
  if (strcmp(name, kPropLocation) == 0) {
    HIRect frame;

    ::HIViewGetFrame(viewRef_, &frame);
    return Location(frame.origin.x, frame.origin.y) - GetViewOffset();
  }
  if (strcmp(name, kPropSize) == 0) {
    return GetSize() - GetInset();
  }
  if (strcmp(name, kPropVisible) == 0) {
    return (Bool)::HIViewIsVisible(viewRef_);
  }
  return Value();
}

Size Carbon::Control::GetSize() const {
  HIRect frame;

  ::HIViewGetFrame(viewRef_, &frame);
  return Size(frame.size.width, frame.size.height);
}

void Carbon::Button::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(Entity::kPropText))
    title.Set(
        CFStringFromString(properties[Entity::kPropText].Coerce<String>()),
        kDontRetain);

  const Rect defaultBounds = { 0, 0, 20, 50 };

  ::CreatePushButtonControl(NULL, &defaultBounds, title, &viewRef_);
}

Bool Carbon::Button::SetProperty(PropertyName name, const Value &value) {
  return Control::SetProperty(name, value);
}

Value Carbon::Button::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(12, 12, 12, 12);
  }
  return Control::GetProperty(name);
}

void Carbon::Checkbox::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(Entity::kPropText))
    title.Set(
        CFStringFromString(properties[Entity::kPropText].Coerce<String>()),
        kDontRetain);

  const Rect defaultBounds = { 0, 0, 20, 50 };

  ::CreateCheckBoxControl(NULL, &defaultBounds, title, 0, true, &viewRef_);
}

void Carbon::Label::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(Entity::kPropText))
    title.Set(
        CFStringFromString(properties[Entity::kPropText].Coerce<String>()),
        kDontRetain);

  const Rect defaultBounds = { 0, 0, 20, 50 };

  ::CreateStaticTextControl(NULL, &defaultBounds, title, NULL, &viewRef_);
}

Value Carbon::Label::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(8, 8, 8, 8);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    float wrapWidth = 0;
    Bool variable = false;

#if 0  // TODO(catmull): variable height
    GetControlProperty(viewRef_, kFenSig, 'VHgt', variable);
    if (variable)
      wrapWidth = GetSize().width;
#endif

    ControlSize size = kControlSizeNormal;
    ThemeFontID fontID = kThemeSystemFont;

    ::GetControlData(
        viewRef_, kControlEntireControl, kControlSizeTag,
        sizeof(size), &size, NULL);
    switch (size) {
      case kControlSizeSmall: fontID = kThemeSmallSystemFont; break;
      case kControlSizeMini:  fontID = kThemeMiniSystemFont;  break;
    }

    // HIViewGetOptimalBounds for static text only adjusts the height,
    // so we calculate the text width manually
    HIThemeTextInfo info = {
      0, kThemeStateActive, fontID,
      kHIThemeTextHorizontalFlushLeft,
      kHIThemeTextVerticalFlushTop,
      0, kHIThemeTextTruncationNone, 0, false };
    CFStringRef title = ::HIViewCopyText(viewRef_);
    float width, height;

    ::HIThemeGetTextDimensions(title, wrapWidth, &info, &width, &height, NULL);
    ::CFRelease(title);
    return Size(width, variable ? std::max(height, 16.0f) : GetSize().height);
  }
  return Control::GetProperty(name);
}

void Carbon::EditField::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(Entity::kPropText))
    title.Set(
        CFStringFromString(properties[Entity::kPropText].Coerce<String>()),
        kDontRetain);

  const Rect defaultBounds = { 0, 0, 20, 50 };

  ::CreateEditUnicodeTextControl(
      NULL, &defaultBounds, title, false, NULL, &viewRef_);
}

void Carbon::Separator::InitializeProperties(const PropertyMap &properties) {
  const Rect defaultBounds = { 0, 0, 2, 50 };

  ::CreateSeparatorControl(NULL, &defaultBounds, &viewRef_);
}

void Carbon::Separator::Finalize() {
  Layout *layout = entity_->GetLayout();

  if (layout == NULL)
    return;

  LayoutContainer *layoutParent = layout->GetLayoutParent();

  if (layoutParent == NULL)
    return;
  if (layoutParent->GetDirection() == LayoutContainer::kLayoutRow)
    layout->SetVSizeOption(kSizeFill);
  else  // kLayoutColumn
    layout->SetHSizeOption(kSizeFill);
}

Value Carbon::Separator::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Size(1, 1);
  }
  return Control::GetProperty(name);
}

}  // namespace Diadem
