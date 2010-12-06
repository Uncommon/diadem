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

#include "Diadem/Test/WindowTestBase.h"
#include "Diadem/Factory.h"

#ifndef DIADEM_TEST_PARSER
#include "Diadem/LibXMLParser.h"
#define DIADEM_TEST_PARSER Diadem::LibXMLParser
#endif

#ifndef DIADEM_PLATFORM

#if TARGET_OS_MAC
#if 0
#include "Diadem/NativeCarbon.h"
#define DIADEM_PLATFORM Diadem::Carbon
#else
#include "Diadem/NativeCocoa.h"
#define DIADEM_PLATFORM Diadem::Cocoa
#endif
#endif

#endif  // DIADEM_PLATFORM

void WindowTestBase::TearDown() {
  delete windowObject_;
}

void WindowTestBase::ReadWindowData_(const char *data) {
  Diadem::Factory factory;

  DIADEM_PLATFORM::SetUpFactory(&factory);

  // register test classes
  factory.Register<Diadem::Entity>("entity");

  ASSERT_TRUE(factory.IsRegistered("window"));
  ASSERT_TRUE(factory.IsRegistered("button"));
  ASSERT_TRUE(factory.IsRegistered("label"));
  ASSERT_TRUE(factory.IsRegistered("entity"));

  DIADEM_TEST_PARSER parser(factory);
  windowRoot_ = parser.LoadEntityFromData(data);

  ASSERT_FALSE(windowRoot_ == NULL);
  ASSERT_FALSE(windowRoot_->GetNative() == NULL);
  ASSERT_FALSE(windowRoot_->GetNative()->GetWindowInterface() == NULL);
  windowObject_ = new Diadem::Window(windowRoot_);
  if (windowObject_->ShowModeless()) {
    windowRoot_->SetProperty(Diadem::kPropLocation, Diadem::Location(20, 50));
    readSucceeded_ = true;
  }
}
