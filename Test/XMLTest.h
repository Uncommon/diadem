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

namespace Diadem {
class Factory;
class Parser;
}

// This is abstracted here so that the same tests can be run with multiple
// parsers. Create a subclass of XMLTest, and use the XML_TESTS macro with the
// name of your subclass.
class XMLTest : public testing::Test {
 public:
  virtual Diadem::Parser* MakeParser(Diadem::Factory &factory) = 0;

  void SimpleTest();
  void NestedTest();
};

#define XML_TESTS(_subclass_) \
    TEST_F(_subclass_, Simple) { SimpleTest(); } \
    TEST_F(_subclass_, Nested) { NestedTest(); }
