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

#ifndef DIADEM_WINDOW_H_
#define DIADEM_WINDOW_H_

#include "Diadem/Layout.h"
#include "Diadem/Native.h"

namespace Diadem {

class Window {
 public:
  Window() { root_ = NULL; }
  explicit Window(Entity *root) : root_(root) {
    DASSERT(IsValid());
    root->SetWindow(this);
  }
  ~Window() { delete root_; }

  Entity* GetRoot() { return root_; }
  const Entity* GetRoot() const { return root_; }
  bool IsValid() const {
    return (root_ != NULL) && (root_->GetNative() != NULL) &&
        (root_->GetNative()->GetWindowInterface() != NULL);
  }

  bool ShowModeless()
    { return root_->GetNative()->GetWindowInterface()->ShowModeless(); }
  bool Close()
    { return root_->GetNative()->GetWindowInterface()->Close(); }
  bool ShowModal(void *parent)
    { return root_->GetNative()->GetWindowInterface()->ShowModal(parent); }
  bool EndModal()
    { return root_->GetNative()->GetWindowInterface()->EndModal(); }

  // Function to be called when the user attempts to close a window. Return
  // false to disallow closing.
  typedef bool (*CloseCallback)(Window *window, void *data);

  void SetCloseCallback(CloseCallback callback, void *data) {
    close_callback_ = callback;
    close_data_ = data;
  }
  // Calls the close callback and returns the result. Returns true if no
  // callback is set.
  bool AttemptClose() {
    return (close_callback_ == NULL) ?
        true : (*close_callback_)(this, close_data_);
  }

  template <class Platform, class Parser>
  void LoadFromFile(const char *path) {
    Factory factory;

    Platform::SetUpFactory(factory);
    root_ = Parser(factory).LoadEntityFromFile(path);
    DASSERT(IsValid());
  }

 protected:
  Entity *root_;  // Root entity for window content. Owning reference.
  CloseCallback close_callback_;
  void *close_data_;
};

}  // namespace Diadem

#endif  // DIADEM_WINDOW_H_
