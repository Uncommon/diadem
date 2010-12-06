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

#include <Carbon/Carbon.h>

#include "Diadem/Native.h"
#include "Diadem/Window.h"

namespace Diadem {

class Factory;

class Carbon {
 public:
  static void SetUpFactory(Factory *factory);

  class NativeCarbon : public Native {
   public:
    virtual const PlatformMetrics& GetPlatformMetrics() const
      { return metrics_; }
   protected:
    static PlatformMetrics metrics_;
  };

  class Window : public NativeCarbon, public WindowInterface {
   public:
    Window() : windowRef_(NULL), isAlert_(false) {}
    ~Window();

    virtual void InitializeProperties(const PropertyMap &properties);
    WindowInterface* GetWindowInterface() { return this; }

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
    virtual void AddChild(Native *child);

    void* GetNativeRef() { return windowRef_; }

    // WindowInterface
    virtual Bool ShowModeless();
    virtual Bool Close();
    virtual Bool ShowModal(void *parent);
    virtual Bool EndModal();
    virtual Bool SetFocus(Entity *newFocus);

    typedef Diadem::Entity EntityType;
    typedef BorderedContainer LayoutType;

   protected:
    WindowRef windowRef_;
    Bool isAlert_;
  };

  class Control : public NativeCarbon {
   public:
    Control() : viewRef_(NULL) {}
    virtual ~Control();

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;

    void* GetNativeRef() { return viewRef_; }

    typedef Entity EntityType;
    typedef Layout LayoutType;

   protected:
    HIViewRef viewRef_;

    Size GetSize() const;
    virtual Spacing GetInset() const { return Spacing(); }
  };

  class Button : public Control {
   public:
    Button() {}

    virtual void InitializeProperties(const PropertyMap &properties);

    virtual Bool SetProperty(PropertyName name, const Value &value);
    virtual Value GetProperty(PropertyName name) const;
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
  };

  class EditField : public Control {
   public:
    EditField() {}

    virtual void InitializeProperties(const PropertyMap &properties);
  };

  class Separator : public Control {
   public:
    Separator() {}

    virtual void InitializeProperties(const PropertyMap &properties);
    virtual void Finalize();
    virtual Value GetProperty(PropertyName name) const;
  };
};

}  // namespace Diadem
