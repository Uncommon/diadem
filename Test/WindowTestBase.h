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

#include <gtest/gtest.h>

#include "Diadem/Window.h"

class WindowTestBase : public testing::Test {
 public:
  WindowTestBase() : windowObject_(NULL), readSucceeded_(false) {}

 protected:
  Diadem::Window *windowObject_;
  Diadem::Entity *windowRoot_;
  bool readSucceeded_;

  virtual void TearDown();

  void ReadWindowData_(const char *data);
  Diadem::Spacing GetWindowMargins() {
    return windowObject_->GetRoot()->GetProperty(
        Diadem::kPropMargins).Coerce<Diadem::Spacing>();
  }
};

#define ReadWindowData(_data_) \
  { ReadWindowData_(_data_); \
    if (!readSucceeded_) return; }
