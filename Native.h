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

#ifndef DIADEM_NATIVE_H_
#define DIADEM_NATIVE_H_

#include "Diadem/Entity.h"
#include "Diadem/Factory.h"
#include "Diadem/Value.h"

namespace Diadem {

extern const PropertyName
    kPropButtonType, kPropFile, kPropStyle, kPropTextAlign, kPropUISize,
    kPropURL, kPropValue;

extern const TypeName
    kTypeNameWindow, kTypeNameBox, kTypeNameButton, kTypeNameCheck,
    kTypeNameLabel, kTypeNameLink, kTypeNameEdit, kTypeNamePassword,
    kTypeNamePath, kTypeNameSeparator, kTypeNameImage, kTypeNamePopup,
    kTypeNameItem, kTypeNameCombo, kTypeNameDate, kTypeNameList,
    kTypeNameTabs, kTypeNameTab;

extern const StringConstant kTextAlignLeft, kTextAlignCenter, kTextAlignRight;

extern const StringConstant kUISizeNormal, kUISizeSmall, kUISizeMini;

extern const StringConstant
    kWindowStyleNameClosable, kWindowStyleNameResizable,
    kWindowStyleNameMinimizable;

// Style mask bits for window attributes
enum WindowStyleBit {
  kStyleClosable    = 0x01,
  kStyleResizable   = 0x02,
  kStyleMinimizable = 0x04,
};

class WindowInterface;

// Wrapper for a platform-specific implementation for an Entity
class Native : public EntityDelegate {
 public:
  Native() {}
  virtual ~Native() {}

  // The actual handle to the native window, control, etc.
  virtual void* GetNativeRef() { return NULL; }

  // Notification that a child has been added. Useful for subviews and menu
  // items where OS-specific action needs to be taken.
  virtual void AddChild(Native *child) {}

  // If the object is a superview that contains native objects for child
  // entities, adjustments need to be made to their locations.
  virtual Bool IsSuperview() const { return false; }
  virtual Location GetSubviewAdjustment() const { return Location(); }

  virtual const PlatformMetrics& GetPlatformMetrics() const = 0;
  virtual WindowInterface* GetWindowInterface() { return NULL; }

  typedef Factory::NoLayout LayoutType;

  // Parses a window's style attribute into a combination of WindowStyleBits
  static uint32_t ParseWindowStyle(const char *style);

 protected:
  // Location must be gotten and set relative to the parent container. This
  // convenience function helps with the coordinate conversion.
  Location GetViewOffset() const;
};

enum ButtonType { kAcceptButton, kCancelButton, kOtherButton };

// MessageData holds the information used by a platform class' ShowMessage().
struct MessageData {
  explicit MessageData(const String &message)
      : message_(message),
        show_cancel_(false), show_other_(false), show_suppress_(false),
        suppressed_(false), default_button_(kAcceptButton) {}

  // The message displayed in the window.
  String message_;
  // Button titles, if different from the defaults, which are:
  // "OK", "Cancel", "Don't Save", "Don't show this message again".
  String accept_text_, cancel_text_, other_text_, suppress_text_;
  // True if the button should be shown. The accept button is always shown.
  Bool show_cancel_, show_other_, show_suppress_;
  // True if the user clicked the suppress button.
  Bool suppressed_;
  // Which button should be activated by the enter/return key.
  ButtonType default_button_;
};

class WindowInterface {
 public:
  virtual ~WindowInterface() {}

  // All of the following return true on success, and false if there is any
  // error.

  // Shows the window for use as a modeless dialog.
  virtual Bool ShowModeless() = 0;
  // Hides a modeless window.
  virtual Bool Close() = 0;
  // Shows the window and starts a blocking modal event loop.
  virtual Bool ShowModal(void *parent) = 0;
  // Ends a modal event loop. Returns false if the window is not modal.
  virtual Bool EndModal() = 0;
  // Sets the user focus to the given entity's control.
  virtual Bool SetFocus(Entity *new_focus) = 0;

  // Methods for testing
  virtual Bool TestClose() { return false; }
};

}  // namespace Diadem

#endif  // DIADEM_NATIVE_H_
