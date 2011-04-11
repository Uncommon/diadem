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

#include "NativeWindows.h"

namespace {

// Windows UI guidelines give measurements in Dialog Units (DLU), whose size
// depends on the font size being used.
class DLU {
public:
  static float kPixelsPerUnitX, kPixelsPerUnitY;

  static void Init();
  static void Recalculate();

  // Half dialog units should be rounded down
  static uint32 RoundDown(float value);

  // Pixels to DLU
  static uint32 ToDialogX(uint32 x)
    { return RoundDown(x/kPixelsPerUnitX); }

  static uint32 ToDialogY(uint32 y)
    { return RoundDown(y/kPixelsPerUnitY); }

  // DLU to pixels
  static uint32 FromDialogX(uint32 x)
    { return RoundDown(x*kPixelsPerUnitX); }

  static uint32 FromDialogY(uint32 y)
    { return RoundDown(y*kPixelsPerUnitY); }
  
  static Spacing Spacing(long t, long l, long b, long r) {
    return Diadem::Spacing(
        FromDialogY(t), FromDialogX(l),
        FromDialogY(b), FromDialogX(r));
  }
};

void DLU::Init() {
  if ((kPixelsPerUnitX != 0) || (kPixelsPerUnitY != 0))
    return;

  TEXTMETRIC tm;
  SIZE size;

  ::GetTextMetrics(Win32::GetInstance().GetDC(), &tm);
  ::GetTextExtentPoint32(
      Win32::GetInstance().GetDC(),
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size);

  const uint32 avgWidth = (size.cx/26+1)/2;
  const uint32 avgHeight = (WORD)tm.tmHeight;

  kPixelsPerUnitX = avgWidth / 4.0f;
  kPixelsPerUnitY = avgHeight / 8.0f;
}

void DLU::Recalculate() {
  kPixelsPerUnitX = 0;
  kPixelsPerUnitY = 0;
  Init();
}

uint32 DLU::RoundDown(float value) {
  const float f = floor(value);
  const float fraction = value - f;

  if (fraction <= 0.5)
    return f;
  return ceil(value);
}

float DLU::kPixelsPerUnitX = 0, DLU::kPixelsPerUnitY = 0;

}

namespace Diadem {

bool Windows::Window::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropText) == 0) {
    SetWindowText(window_, value.Coerce<String>().Get());
    return true;
  }
  if (strcmp(name, kPropSize) == 0) {
    Size new_size = value.Coerce<Size>();
    RECT rect = {}
    POINT local = { rect.x, rect.y };

    // GetWindowRect is in screen coordinates, but MoveWindow is
    // relative to the parent
    GetWindowRect(window_, &rect);
    ScreenToClient(GetParent(window_), &local);
    MoveWindow(window_, local.x, local.y, size.width, size.height);
    return true;
  }
  if (strcmp(name, kPropLocation) == 0) {
    const Location loc = value.Coerce<Location>();
    RECT rect;

    GetWindowRect(window_, &rect);
    MoveWindow(
        window_, loc.x, loc.y,
        rect.right - rect.left,
        rect.bottom - rect.top);
    return true;
  }
  return NativeWindows::SetProperty(name, value);
}

Value Windows::Window::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropLocation) == 0) {
    RECT rect = {};
    POINT local = {};

    GetWindowRect(window_, &rect);
    ScreenToClient(GetParent(window_), &local);
    return Value(Location(local.x, local.y));
  }
  if (strcmp(name, kPropText) == 0) {
    return Value(GetText());
  }
  return NativeWindows::GetProperty(name);
}

String Windows::Window::GetText() const {
  // TODO(catmull): handle wide strings
  const int length = GetWindowTextLength(window_);

  if (length == 0)
    return Value(String());
  ++length;

  char *text = new char[length];

  GetWindowText(window_, text, length);
  return String(text, String::kAdoptBuffer);
}

void Windows::AppWindow::InitializeProperties(const PropertyMap &properties) {
  window_ = CreateWindowEx(
      0, window_class, title, WS_POPUP | WS_CAPTION,
      0, 0, 50, 50,
      NULL, NULL, NULL, NULL);
}

bool Windows::AppWindow::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropSize) == 0) {
    RECT client = { 0, 0, 0, 0 }, bounds = { 0, 0, 0, 0 };

    GetWindowRect(window_, &bounds);
    GetClientRect(window_, &client);

    const LONG
        hPad = (bounds.right-bounds.left) - client.right,
        vPad = (bounds.bottom-bounds.top) - client.bottom;

    MoveWindow(
        window_, bounds.left, bounds.top,
        size.width + hPad, size.height + vPad, true);
    return true;
  }
  return Window::SetProperty(name, value);
}

Value Windows::AppWindow::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropSize) == 0) {
      RECT rect;

      GetClientRect(window_, &rect);
      return Value(Size(rect.right - rect.left, rect.bottom - rect.top));
  }
  return Window::GetProperty(name);
}

void Windows::AppWindow::AddChild(Native *child) const {
  SetParent(reinterpret_cast<HWND>(child->GetNativeRef()), window_);
}

bool Windows::AppWindow::ShowModeless() {
  ShowWindow(window_, SW_SHOWNORMAL);
  return true;
}

bool Windows::AppWindow::Close() {
  ShowWindow(window_, SW_HIDE);
  return true;
}

bool Windows::AppWindow::ShowModal(void *parent) {
  // create a custom modal loop
  return true;
}

bool Windows::AppWindow::EndModal() {
  // stop the modal loop
  return true;
}

bool Windows::AppWindow::SetFocus(Entity *new_focus) {
  // TBI
  return false;
}

bool Windows::AppWindow::TestClose() {
  // TBI
  return false;
}

void Windows::Button::InitializeProperties(const PropertyMap &properties) {
  window_ = CreateWindow(
      L"BUTTON", L"", WS_TABSTOP | WS_VISIBLE | WS_CHILD | GetButtonStyle(),
      0, 0, 20, 20,
      NULL, NULL, NULL, NULL);
}

DWORD Windows::PushButton::GetButtonStyle() const {
  return BS_DEFPUSHBUTTON;
}

Value Windows::PushButton::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Value(Size(
        max<uint32_t>(
            DLU::FromDialogX(50),
            GetLabelSize(GetText()).width + DLU::FromDialogX(20)),
        DLU::FromDialogY(14)));
  }
  return Button::GetProperty(name);
}

Value Windows::CheckRadio::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropMinimumSize) == 0) {
    return Value(Size(
        max<uint32_t>(
            DLU::FromDialogX(50),
            GetLabelSize(GetText()).width + DLU::FromDialogX(20)),
        DLU::FromDialogY(14)));
  }
  return Button::GetProperty(name);
}

DWORD Windows::Checkbox::GetButtonStyle() const {
  return BS_AUTOCHECKBOX;
}

DWORD Windows::Radio::GetButtonStyle() const {
  return BS_AUTORADIOBUTTON;
}

}
