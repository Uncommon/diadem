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

Diadem::String StringFromCFString(CFStringRef cf_string) {
  const CFIndex length = ::CFStringGetBytes(
      cf_string, CFRangeMake(0, ::CFStringGetLength(cf_string)),
      kCFStringEncodingUTF8, '.', false, NULL, 0, NULL);

  if (length == 0)
    return Diadem::String();

  char *buffer = new char[length+1];

  ::CFStringGetBytes(
      cf_string, CFRangeMake(0, ::CFStringGetLength(cf_string)),
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
  factory->RegisterNative<Label>(kTypeNameLabel);
  factory->RegisterNative<Window>(kTypeNameWindow);
  factory->RegisterNative<Button>(kTypeNameButton);
  factory->RegisterNative<Checkbox>(kTypeNameCheck);
  factory->RegisterNative<EditField>(kTypeNameEdit);
  factory->RegisterNative<Separator>(kTypeNameSeparator);
}

void Carbon::Window::InitializeProperties(const PropertyMap &properties) {
  const Rect default_bounds = { 40, 0, 50, 50 };
  OSStatus err;

  err = ::CreateNewWindow(
      kMovableModalWindowClass,
      kWindowAsyncDragAttribute |
          kWindowStandardHandlerAttribute |
          kWindowCompositingAttribute,
      &default_bounds, &window_ref_);
  if ((err == noErr) && (window_ref_ != NULL)) {
    if (properties.Exists(kPropText)) {
      const String title_string =
          properties[kPropText].Coerce<String>();
      ScopedCFType<CFStringRef> cf_title(
          CFStringFromString(title_string),
          kDontRetain);

      ::SetWindowTitleWithCFString(window_ref_, cf_title);
    }
  }
}

Carbon::Window::~Window() {
  if (window_ref_ != NULL)
    ::CFRelease(window_ref_);
}

bool Carbon::Window::SetProperty(PropertyName name, const Value &value) {
  if (window_ref_ == NULL)
    return false;
  if (strcmp(name, kPropText) == 0) {
    ScopedCFType<CFStringRef> title(
        CFStringFromString(value.Coerce<String>()),
        kDontRetain);
    ::SetWindowTitleWithCFString(window_ref_, title);
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>();
    Rect bounds;

    ::GetWindowBounds(window_ref_, kWindowContentRgn, &bounds);
    bounds.right = bounds.left + size.width;
    bounds.bottom = bounds.top + size.height;
    ::SetWindowBounds(window_ref_, kWindowContentRgn, &bounds);
    // TODO(catmull): reposition?
    return true;
  }
  return false;
}

Value Carbon::Window::GetProperty(PropertyName name) const {
  if (window_ref_ == NULL)
    return false;
  if (strcmp(name, kPropText) == 0) {
    ScopedCFType<CFStringRef> title;

    ::CopyWindowTitleAsCFString(window_ref_, title.RetainedOutPtr());
    return StringFromCFString(title);
  }
  if (strcmp(name, kPropSize) == 0) {
    Rect bounds;

    ::GetWindowBounds(window_ref_, kWindowContentRgn, &bounds);
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

  HIViewRef content_view = NULL;

  ::HIViewFindByID(
      ::HIViewGetRoot(window_ref_), kHIViewWindowContentID, &content_view);
  if (content_view != NULL)
    ::HIViewAddSubview(content_view, (HIViewRef)child->GetNativeRef());
}

bool Carbon::Window::ShowModeless() {
  if (window_ref_ == NULL)
    return false;
  ::SelectWindow(window_ref_);
  ::ShowWindow(window_ref_);
  return true;
}

bool Carbon::Window::Close() {
  if (window_ref_ == NULL)
    return false;
  ::HideWindow(window_ref_);
  return true;
}

bool Carbon::Window::ShowModal(void *on_parent) {
  if (window_ref_ == NULL)
    return false;

  // TODO(catmull): defer to main thread

  WindowPtr parent = ::GetFrontWindowOfClass(kDocumentWindowClass, true);
  WindowPositionMethod window_position;

  if (parent == NULL)
    parent = ::GetFrontWindowOfClass(kMovableModalWindowClass, true);
  if (parent != NULL) {
    // We use alert position in both cases because that's actually the
    // preferred location for new windows in the HIG. In the case where it
    // has been marked as an alert and has a parent window, we assume it's
    // strongly associated with that window (i.e., it would be a sheet if
    // not for our modality needs), so we give it window alert position.
    window_position = is_alert_ ? kWindowAlertPositionOnParentWindow
                              : kWindowAlertPositionOnParentWindowScreen;
  } else {
    window_position = kWindowAlertPositionOnMainScreen;
  }
  ::RepositionWindow(window_ref_, parent, window_position);
  ::SetThemeCursor(kThemeArrowCursor);
  ::ShowWindow(window_ref_);
  ::SelectWindow(window_ref_);
  ::RunAppModalLoopForWindow(window_ref_);
  return true;
}

bool Carbon::Window::EndModal() {
  if (window_ref_ == NULL)
    return false;

  // TODO(catmull): defer to main thread

  // TODO(catmull): maybe check window modality for good measure

  ::QuitAppModalLoopForWindow(window_ref_);
  ::HideWindow(window_ref_);
  return true;
}

bool Carbon::Window::SetFocus(Entity *new_focus) {
  if ((window_ref_ == NULL) || (new_focus == NULL) ||
      (new_focus->GetNative() == NULL) ||
      (new_focus->GetNative()->GetNativeRef() == NULL))
    return false;
  if (!::IsValidControlHandle((HIViewRef)
      new_focus->GetNative()->GetNativeRef()))
    return false;

  ::SetKeyboardFocus(
      window_ref_,
      (HIViewRef)new_focus->GetNative()->GetNativeRef(),
      kControlFocusNextPart);
  return true;
}

Carbon::Control::~Control() {
  if (view_ref_ != NULL)
    ::CFRelease(view_ref_);
}

bool Carbon::Control::SetProperty(PropertyName name, const Value &value) {
  if (view_ref_ == NULL)
    return false;
  if (strcmp(name, kPropLocation) == 0) {
    const Location loc = value.Coerce<Location>() + GetViewOffset();

    ::HIViewPlaceInSuperviewAt(view_ref_, loc.x, loc.y);
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    const Size size = value.Coerce<Size>() + GetInset();
    HIRect frame;

    ::HIViewGetFrame(view_ref_, &frame);
    frame.size.width = size.width;
    frame.size.height = size.height;
    ::HIViewSetFrame(view_ref_, &frame);
    return true;
  }
  if (strcmp(name, kPropText) == 0) {
    ScopedCFType<CFStringRef> cf_text(
        CFStringFromString(value.Coerce<String>()),
        kDontRetain);

    return ::HIViewSetText(view_ref_, cf_text) == noErr;
  }
  if (strcmp(name, kPropVisible) == 0) {
    ::HIViewSetVisible(view_ref_, value.Coerce<bool>());
  }
  return false;
}

Value Carbon::Control::GetProperty(PropertyName name) const {
  if (view_ref_ == NULL)
    return Value();
  if (strcmp(name, kPropText) == 0) {
    ScopedCFType<CFStringRef> cf_text(::HIViewCopyText(view_ref_), kDontRetain);

    return StringFromCFString(cf_text);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    HIRect bounds;

    ::HIViewGetOptimalBounds(view_ref_, &bounds, NULL);
    return Size(bounds.size.width, bounds.size.height) - GetInset();
  }
  if (strcmp(name, kPropLocation) == 0) {
    HIRect frame;

    ::HIViewGetFrame(view_ref_, &frame);
    return Location(frame.origin.x, frame.origin.y) - GetViewOffset();
  }
  if (strcmp(name, kPropSize) == 0) {
    return GetSize() - GetInset();
  }
  if (strcmp(name, kPropVisible) == 0) {
    return (bool)::HIViewIsVisible(view_ref_);
  }
  return Value();
}

Size Carbon::Control::GetSize() const {
  HIRect frame;

  ::HIViewGetFrame(view_ref_, &frame);
  return Size(frame.size.width, frame.size.height);
}

void Carbon::Button::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(kPropText))
    title.Set(
        CFStringFromString(properties[kPropText].Coerce<String>()),
        kDontRetain);

  const Rect default_bounds = { 0, 0, 20, 50 };

  ::CreatePushButtonControl(NULL, &default_bounds, title, &view_ref_);
}

bool Carbon::Button::SetProperty(PropertyName name, const Value &value) {
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

  if (properties.Exists(kPropText))
    title.Set(
        CFStringFromString(properties[kPropText].Coerce<String>()),
        kDontRetain);

  const Rect default_bounds = { 0, 0, 20, 50 };

  ::CreateCheckBoxControl(NULL, &default_bounds, title, 0, true, &view_ref_);
}

void Carbon::Label::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(kPropText))
    title.Set(
        CFStringFromString(properties[kPropText].Coerce<String>()),
        kDontRetain);

  const Rect default_bounds = { 0, 0, 20, 50 };

  ::CreateStaticTextControl(NULL, &default_bounds, title, NULL, &view_ref_);
}

Value Carbon::Label::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing(8, 8, 8, 8);
  }
  if (strcmp(name, kPropMinimumSize) == 0) {
    float wrap_width = 0;
    bool variable = false;

#if 0  // TODO(catmull): variable height
    GetControlProperty(view_ref_, kFenSig, 'VHgt', variable);
    if (variable)
      wrap_width = GetSize().width;
#endif

    ControlSize size = kControlSizeNormal;
    ThemeFontID font_ID = kThemeSystemFont;

    ::GetControlData(
        view_ref_, kControlEntireControl, kControlSizeTag,
        sizeof(size), &size, NULL);
    switch (size) {
      case kControlSizeSmall: font_ID = kThemeSmallSystemFont; break;
      case kControlSizeMini:  font_ID = kThemeMiniSystemFont;  break;
    }

    // HIViewGetOptimalBounds for static text only adjusts the height,
    // so we calculate the text width manually
    HIThemeTextInfo info = {
      0, kThemeStateActive, font_ID,
      kHIThemeTextHorizontalFlushLeft,
      kHIThemeTextVerticalFlushTop,
      0, kHIThemeTextTruncationNone, 0, false };
    CFStringRef title = ::HIViewCopyText(view_ref_);
    float width, height;

    ::HIThemeGetTextDimensions(title, wrap_width, &info, &width, &height, NULL);
    ::CFRelease(title);
    return Size(width, variable ? std::max(height, 16.0f) : GetSize().height);
  }
  return Control::GetProperty(name);
}

void Carbon::EditField::InitializeProperties(const PropertyMap &properties) {
  ScopedCFType<CFStringRef> title;

  if (properties.Exists(kPropText))
    title.Set(
        CFStringFromString(properties[kPropText].Coerce<String>()),
        kDontRetain);

  const Rect default_bounds = { 0, 0, 20, 50 };

  ::CreateEditUnicodeTextControl(
      NULL, &default_bounds, title, false, NULL, &view_ref_);
}

void Carbon::Separator::InitializeProperties(const PropertyMap &properties) {
  const Rect default_bounds = { 0, 0, 2, 50 };

  ::CreateSeparatorControl(NULL, &default_bounds, &view_ref_);
}

void Carbon::Separator::Finalize() {
  Layout *layout = entity_->GetLayout();

  if (layout == NULL)
    return;
  if (layout->GetDirection() == Layout::kLayoutRow)
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
