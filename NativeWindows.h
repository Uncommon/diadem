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

namespace Diadem {

class Factory;

class Windows {
 public:
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

  // Abstract superclass for all Windows native classes
  class NativeWindows : public Native {
   public:
    virtual const PlatformMetrics& GetPlatformMetrics() const
      { return metrics_; }
   protected:
    static PlatformMetrics metrics_;
  };

  // On Windows, a "window" can be a control as well as an application window.
  class Window : public NativeWindows {
   public:
    Window() {}
    virtual ~Window() {}

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

   protected:
    HWND window_;

    Size GetLabelSize(const String &text) const;
    String GetText() const;
  };

  // <window> implementation
  class AppWindow : public Window, public WindowInterface {
   public:
    AppWindow();
    ~AppWindow();

    virtual void InitializeProperties(const PropertyMap &properties);
    WindowInterface* GetWindowInterface() { return this; }

    virtual String GetTypeName() const { return kTypeNameWindow; }

    virtual bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
    virtual void AddChild(Native *child);

    void* GetNativeRef() { return window_; }

    // WindowInterface
    virtual bool ShowModeless();
    virtual bool Close();
    virtual bool ShowModal(void *parent);
    virtual bool EndModal();
    virtual bool SetFocus(Entity *new_focus);
    virtual bool TestClose();

    typedef Diadem::RootEntity EntityType;
    typedef BorderedContainer LayoutType;
  };

  // Base class for push button, checkbox, radio
  class Button : public Window {
   public:
    Button() {}
    ~Button() {}

    virtual void InitializeProperties(const PropertyMap &properties);

   protected:
    virtual DWORD GetButtonStyle() const = 0;
  };

  // <button> implementation
  class PushButton : public Button {
   public:
    PushButton() {}

    virtual Value GetProperty(PropertyName name) const;

   protected:
    virtual DWORD GetButtonStyle() const;
  };

  // Checkboxes and radios use the same size calculations
  class CheckRadio : public Button {
   public:
    CheckRadio() {}

    virtual Value GetProperty(PropertyName name) const;
  };

  // <check> implementation
  class Checkbox : public Button {
   public:
    Checkbox() {}

   protected:
    virtual DWORD GetButtonStyle() const;
  };

  // <radio> implementation
  class Radio : public Button {
   public:
    Radio() {}

   protected:
    virtual DWORD GetButtonStyle() const;
  };

  // Label and separator are both static controls
  class Static : public Window {
   public:
    Static() {}
  };

  // <label> implementation
  class Label : public Static {
   public:
    Label();

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameLabel; }
    virtual Value GetProperty(PropertyName name) const;
    virtual bool SetProperty(PropertyName name, const Value &value);
  };

  // <separator> implementation
  class Separator : public Static {
   public:
    Separator() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual String GetTypeName() const { return kTypeNameSeparator; }
  };
};

}  // namespace Diadem
