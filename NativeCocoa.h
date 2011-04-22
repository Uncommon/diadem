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

#include "Diadem/Native.h"
#include "Diadem/Window.h"

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
@class ButtonTarget;
@class TableDelegate;
@class WindowDelegate;
#else
typedef void
    NSWindow, NSView, NSMenuItem, NSAlert, NSTableView, NSTableColumn, *Class;
typedef void ButtonTarget, TableDelegate, WindowDelegate;
struct NSPoint { CGFloat x, y; };
typedef unsigned int NSButtonType;
#endif

class CocoaTest;

namespace Diadem {

class Factory;

class Cocoa {
 public:
  static String GetFullPath(const char *filename);
  static void SetUpFactory(Factory *factory);

  // Displays a dialog to ask the user to choose a folder. Returns an empty
  // string if the user cancels.
  static String ChooseFolder(const String &initial_path);

  // Displays a "Save As" type dialog.
  static String ChooseNewPath(
      const String &prompt,
      const String &initial_path,
      const String &initial_name);

  static ButtonType ShowMessage(MessageData *message);

  // Abstract superclass for all Cocoa native classes
  class NativeCocoa : public Native {
   public:
    virtual const PlatformMetrics& GetPlatformMetrics() const
      { return metrics_; }
   protected:
    static PlatformMetrics metrics_;
  };

  // <window> implementation
  class Window : public NativeCocoa, public WindowInterface {
   public:
    Window();
    ~Window();

    virtual void InitializeProperties(const PropertyMap &properties);
    WindowInterface* GetWindowInterface() { return this; }

    virtual String GetTypeName() const { return kTypeNameWindow; }

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
    virtual void AddChild(Native *child);

    void* GetNativeRef() { return window_ref_; }

    // WindowInterface
    virtual bool ShowModeless();
    virtual bool Close();
    virtual bool ShowModal(void *parent);
    virtual bool EndModal();
    virtual bool SetFocus(Entity *new_focus);
    virtual bool TestClose();

    typedef Diadem::RootEntity EntityType;
    typedef BorderedContainer LayoutType;

   protected:
    NSWindow *window_ref_;
    WindowDelegate *delegate_;
  };

  // Base class for NSView wrappers
  class View : public NativeCocoa {
   public:
    View() : view_ref_(NULL) {}
    virtual ~View();

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    void* GetNativeRef() { return view_ref_; }

    typedef Entity EntityType;
    typedef Layout LayoutType;

   protected:
    NSView *view_ref_;

    // Performs setup needed by all subclasses
    void ConfigureView();
    // Returns the Diadem layout location of the view.
    Location GetLocation() const;
    // Returns the size of the view without compensation for inset etc.
    Size GetViewSize() const;
    // Flips point.y to compensate for inverted coordinates
    NSPoint InvertPoint(NSPoint point) const;
    // Returns the difference between the view bounds and the layout bounds
    virtual Spacing GetInset() const { return Spacing(); }
  };

  // <box> implementation
  class Box : public View {
   public:
    Box() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameBox; }
    virtual Value GetProperty(PropertyName name) const;
    virtual Spacing GetInset() const;
    // Adds children as subviews
    virtual void AddChild(Native *child);
    // A box is a native superview
    virtual bool IsSuperview() const { return true; }
    // Returns an offset to compensate for the box's metrics
    virtual Location GetSubviewAdjustment() const;

    typedef BorderedContainer LayoutType;
  };

  // Base class for NSControl wrappers
  class Control : public View {
   public:
    Control() {}
    virtual ~Control() {}

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
  };

  // Base class for pushbutton, checkbox, etc.
  class Button : public Control {
   public:
    Button();
    ~Button();

    virtual bool SetProperty(PropertyName name, const Value &value);

 protected:
    ButtonTarget *target_;

    virtual void MakeTarget();
  };

  // <button> implementation
  class PushButton : public Button {
   public:
    PushButton() {}

    virtual void InitializeProperties(const PropertyMap &properties);

    virtual String GetTypeName() const { return kTypeNameButton; }

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    virtual Spacing GetInset() const;
  };

  // Base class for checkboxes and radio buttons
  class ValueButton : public Button {
   public:
    ValueButton() {}
    virtual ~ValueButton() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual void Finalize();
    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

   protected:
    virtual NSButtonType GetButtonType() const = 0;
  };

  // <check> implementation
  class Checkbox : public ValueButton {
   public:
    Checkbox() {}

    virtual Value GetProperty(PropertyName name) const;

    virtual String GetTypeName() const { return kTypeNameCheck; }

   protected:
    Spacing GetInset() const { return Spacing(-2, -2, -2, 0); }
    NSButtonType GetButtonType() const;
  };

  // <radio> implementation
  class Radio : public ValueButton {
   public:
    Radio() {}

    virtual Value GetProperty(PropertyName name) const;

    virtual String GetTypeName() const { return kTypeNameRadio; }

   protected:
    Spacing GetInset() const;
    NSButtonType GetButtonType() const;
  };

  // <label> implementation
  class Label : public Control {
   public:
    Label();

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameLabel; }
    virtual Value GetProperty(PropertyName name) const;
    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual void Finalize();

    virtual Spacing GetInset() const;

   protected:
    uint32_t ui_size_;
    bool heading_;

    void UpdateFont();
  };

  // <link> implementation
  class Link : public Label {
   public:
    Link() {}

    void InitializeProperties(const PropertyMap &properties);
    String GetTypeName() const { return kTypeNameLink; }
    bool SetProperty(PropertyName name, const Value &value);

    void SetURL(const String &url);
  };

  // <edit> implementation
  class EditField : public Control {
   public:
    EditField() {}

    void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameEdit; }
    Value GetProperty(PropertyName name) const;
    bool SetProperty(PropertyName name, const Value &value);

   protected:
    virtual Class GetTextFieldClass();
  };

  // <password> implementation
  class PasswordField : public EditField {
   public:
    PasswordField() {}

    virtual String GetTypeName() const { return kTypeNamePassword; }

   protected:
    virtual Class GetTextFieldClass();
  };

  // <path> implementation
  class PathBox : public View {
   public:
    PathBox() {}

    void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNamePath; }
    Value GetProperty(PropertyName name) const;
    bool SetProperty(PropertyName name, const Value &value);
  };

  // <separator> implementation
  class Separator : public View {
   public:
    Separator() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameSeparator; }
    virtual void Finalize();
    virtual Value GetProperty(PropertyName name) const;
  };

  // <image> implementation
  class Image : public Control {
   public:
    Image() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameImage; }
    virtual Value GetProperty(PropertyName name) const;
  };

  // <appicon> implementation
  class AppIcon : public Control {
   public:
    AppIcon() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameAppIcon; }
    virtual Value GetProperty(PropertyName name) const;
  };

  // <popup> implementation
  class Popup : public Button {
   public:
    Popup() {}

    void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNamePopup; }
    void AddChild(Native *child);
    Value GetProperty(PropertyName name) const;
    bool SetProperty(PropertyName name, const Value &value);

    Spacing GetInset() const;
  };

  // <item> implementation
  class PopupItem : public NativeCocoa {
   public:
    PopupItem() : item_(NULL) {}

    void InitializeProperties(const PropertyMap &properties);
    String GetTypeName() const { return kTypeNameItem; }
    void* GetNativeRef() { return item_; }
    bool SetProperty(PropertyName name, const Value &value);
    Value GetProperty(PropertyName name) const;

    typedef Entity EntityType;

   protected:
    NSMenuItem *item_;
  };

  // <slider> implementation
  class Slider : public Control {
   public:
    Slider() {}

    void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameSlider; }
    Value GetProperty(PropertyName name) const;
    bool SetProperty(PropertyName name, const Value &value);

    Spacing GetInset() const;
  };

  // <list> implementation
  class List : public View {
   public:
    List();
    ~List();

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameList; }
    virtual void AddChild(Native *child);
    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    ListDataInterface* GetData() { return data_; }
    uint32_t GetRowCount() const { return row_count_; }

   protected:
    NSTableView *table_view_;
    TableDelegate *table_delegate_;
    ListDataInterface *data_;
    uint32_t row_count_;
  };

  // <column> implementation
  class ListColumn : public NativeCocoa {
   public:
    ListColumn();

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameColumn; }
    virtual void* GetNativeRef() { return column_ref_; }
    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    typedef Entity EntityType;

   protected:
    NSTableColumn *column_ref_;
    ExplicitSize size_;  // Only width is used.
  };

 protected:
  friend class ::CocoaTest;
  static NSAlert* AlertForMessageData(MessageData *message);
};

}  // namespace Diadem
