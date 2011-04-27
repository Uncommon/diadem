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
    kPropButtonType, kPropColumnType, kPropData, kPropFile, kPropMax, kPropMin,
    kPropRowCount, kPropStyle, kPropTextAlign, kPropTicks, kPropUISize,
    kPropURL, kPropValue;

extern const TypeName
    kTypeNameAppIcon, kTypeNameBox, kTypeNameButton, kTypeNameCheck,
    kTypeNameColumn, kTypeNameCombo, kTypeNameDate, kTypeNameEdit,
    kTypeNameImage, kTypeNameItem, kTypeNameLabel, kTypeNameLink, kTypeNameList,
    kTypeNamePassword, kTypeNamePath, kTypeNamePopup, kTypeNameRadio,
    kTypeNameRadioGroup, kTypeNameSeparator, kTypeNameSlider, kTypeNameTab,
    kTypeNameTabs, kTypeNameWindow;

extern const StringConstant kTextAlignLeft, kTextAlignCenter, kTextAlignRight;

extern const StringConstant kUISizeNormal, kUISizeSmall, kUISizeMini;

extern const StringConstant
    kWindowStyleNameClosable, kWindowStyleNameResizable,
    kWindowStyleNameMinimizable;

extern const StringConstant
    kButtonTypeNameDefault, kButtonTypeNameCancel;

extern const StringConstant kLabelStyleNameHead;

extern const StringConstant kColumnTypeNameText, kColumnTypeNameCheck;

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
  virtual bool IsSuperview() const { return false; }
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

// A radio group's value is the index of its child whose value is non-zero.
// Children are usually radio buttons, or they could be groups whose first child
// is a radio button.
class RadioGroup : public Entity {
 public:
  RadioGroup() {}

  virtual void InitializeProperties(
      const PropertyMap &properties,
      const Factory &factory);
  virtual void Finalize();

  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  virtual void ChildValueChanged(Entity *child);

  virtual String GetTypeName() const { return kTypeNameRadioGroup; }

 protected:
  void SetSelectedIndex(uint32_t index);
};

enum ButtonType { kAcceptButton, kCancelButton, kOtherButton };

// MessageData holds the information used by a platform class' ShowMessage().
struct MessageData {
  explicit MessageData(const String &message)
      : message_(message),
        show_cancel_(false), show_other_(false), show_suppress_(false),
        suppressed_(false), default_button_(kAcceptButton),
        callback_(NULL), callback_data_(NULL) {}

  // If provided, the callback will be called after the message is dismissed.
  // The callback may not be called on the same thread that called ShowMessage.
  typedef void (*Callback)(ButtonType button, void *data);

  // The message displayed in the window.
  String message_;
  // Button titles, if different from the defaults, which are:
  // "OK", "Cancel", "Don't Save", "Don't show this message again".
  String accept_text_, cancel_text_, other_text_, suppress_text_;
  // True if the button should be shown. The accept button is always shown.
  bool show_cancel_, show_other_, show_suppress_;
  // True if the user clicked the suppress button.
  bool suppressed_;
  // Which button should be activated by the enter/return key.
  ButtonType default_button_;
  // Function to be called when the dialog is dismissed.
  Callback callback_;
  // Data to be passed to the callback.
  void *callback_data_;
};

class WindowInterface {
 public:
  virtual ~WindowInterface() {}

  // All of the following return true on success, and false if there is any
  // error.

  // Shows the window for use as a modeless dialog.
  virtual bool ShowModeless() = 0;
  // Hides a modeless window.
  virtual bool Close() = 0;
  // Shows the window and starts a blocking modal event loop.
  virtual bool ShowModal(void *parent) = 0;
  // Ends a modal event loop. Returns false if the window is not modal.
  virtual bool EndModal() = 0;
  // Sets the user focus to the given entity's control.
  virtual bool SetFocus(Entity *new_focus) = 0;

  // Methods for testing
  virtual bool TestClose() { return false; }
};

// Callback interface for supplying list data
class ListDataInterface {
 public:
  virtual ~ListDataInterface() {}

  // Returns the text content for the given row and column.
  virtual String GetCellText(uint32_t row, const char *column) const = 0;
  // The user has clicked the row's checkbox.
  virtual void SetRowChecked(uint32_t row, bool check) = 0;
  // Returns true if the given row should be checked.
  virtual bool GetRowChecked(uint32_t row) const = 0;
  // Notification that the list object has been deleted. The data object may
  // delete itself inside this callback.
  virtual void ListDeleted() {}
};

}  // namespace Diadem

#endif  // DIADEM_NATIVE_H_
