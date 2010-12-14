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
#else
typedef void NSWindow, NSView, NSMenuItem, NSAlert, *Class;
typedef void ButtonTarget;
struct NSPoint { CGFloat x, y; };
#endif

class CocoaTest;

namespace Diadem {

class Factory;

class Cocoa {
 public:
  static void SetUpFactory(Factory *factory);

  // Displays a dialog to ask the user to choose a folder. Returns an empty
  // string if the user cancels.
  static String ChooseFolder(const String &initial_path);

  static ButtonType ShowMessage(MessageData *message);

  class NativeCocoa : public Native {
   public:
    virtual const PlatformMetrics& GetPlatformMetrics() const
      { return metrics_; }
   protected:
    static PlatformMetrics metrics_;
  };

  class Window : public NativeCocoa, public WindowInterface {
   public:
    Window() : window_ref_(NULL), is_alert_(false) {}
    ~Window();

    virtual void InitializeProperties(const PropertyMap &properties);
    WindowInterface* GetWindowInterface() { return this; }

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
    virtual void AddChild(Native *child);

    void* GetNativeRef() { return window_ref_; }

    // WindowInterface
    virtual Bool ShowModeless();
    virtual Bool Close();
    virtual Bool ShowModal(void *parent);
    virtual Bool EndModal();
    virtual Bool SetFocus(Entity *new_focus);

    typedef Diadem::Entity EntityType;
    typedef BorderedContainer LayoutType;

   protected:
    NSWindow *window_ref_;
    Bool is_alert_;
  };

  class View : public NativeCocoa {
   public:
    View() : view_ref_(NULL) {}
    virtual ~View();

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    void* GetNativeRef() { return view_ref_; }

    typedef Entity EntityType;
    typedef Layout LayoutType;

   protected:
    NSView *view_ref_;

    void ConfigureView();
    Location GetLocation() const;
    Size GetViewSize() const;
    NSPoint InvertPoint(NSPoint point) const;
    virtual Spacing GetInset() const { return Spacing(); }
  };

  class Box : public View {
   public:
    Box() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual Value GetProperty(PropertyName name) const;
    virtual Spacing GetInset() const;
    virtual void AddChild(Native *child);
    virtual Bool IsSuperview() const { return true; }
    virtual Location GetSubviewAdjustment() const;

    typedef BorderedContainer LayoutType;
  };

  class Control : public View {
   public:
    Control() {}
    virtual ~Control() {}

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
  };

  class Button : public Control {
   public:
    Button() : target_(NULL) {}
    ~Button();

    virtual void InitializeProperties(const PropertyMap &properties);

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    virtual Spacing GetInset() const;

   protected:
    ButtonTarget *target_;
  };

  class Checkbox : public Control {
   public:
    Checkbox() {}

    virtual void InitializeProperties(const PropertyMap &properties);

   protected:
    Spacing GetInset() const { return Spacing(2, 2, 2, 0); }
  };

  class Label : public Control {
   public:
    Label() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual Value GetProperty(PropertyName name) const;
    virtual Bool SetProperty(PropertyName name, const Value &value);

    virtual Spacing GetInset() const;
  };

  class Link : public Label {
   public:
    Link() {}

    void InitializeProperties(const PropertyMap &properties);
    Bool SetProperty(PropertyName name, const Value &value);

    void SetURL(const String &url);
  };

  class EditField : public Control {
   public:
    EditField() {}

    void InitializeProperties(const PropertyMap &properties);
    Value GetProperty(PropertyName name) const;
    Bool SetProperty(PropertyName name, const Value &value);

   protected:
    virtual Class GetTextFieldClass();
  };

  class PasswordField : public EditField {
   public:
    PasswordField() {}

   protected:
    virtual Class GetTextFieldClass();
  };

  class PathBox : public View {
   public:
    PathBox() {}

    void InitializeProperties(const PropertyMap &properties);
    Value GetProperty(PropertyName name) const;
    Bool SetProperty(PropertyName name, const Value &value);
  };

  class Separator : public Control {
   public:
    Separator() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual void Finalize();
    virtual Value GetProperty(PropertyName name) const;
  };

  class Image : public Control {
   public:
    Image() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual Value GetProperty(PropertyName name) const;
  };

  class Popup : public Control {
   public:
    Popup() {}

    void InitializeProperties(const PropertyMap &properties);
    void AddChild(Native *child);
    Value GetProperty(PropertyName name) const;

    Spacing GetInset() const;
  };

  class PopupItem : public NativeCocoa {
   public:
    PopupItem() : item_(NULL) {}

    void InitializeProperties(const PropertyMap &properties);
    void* GetNativeRef() { return item_; }
    Bool SetProperty(PropertyName name, const Value &value);
    Value GetProperty(PropertyName name) const;

    typedef Entity EntityType;

   protected:
    NSMenuItem *item_;
  };

 protected:
  friend class ::CocoaTest;
  static NSAlert* AlertForMessageData(MessageData *message);
};

}  // namespace Diadem
