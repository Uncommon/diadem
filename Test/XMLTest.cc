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

#include "XMLTest.h"
#include "Diadem/Factory.h"

// Loads a single entity from XML
void XMLTest::SimpleTest() {
  Diadem::Factory factory;

  factory.Register<Diadem::Entity>("entity");

  Diadem::Parser *parser = MakeParser(factory);
  Diadem::Entity *result = parser->LoadEntityFromData("<entity name='Sam'/>");

  ASSERT_NE((Diadem::Entity*)NULL, result);
  EXPECT_STREQ("Sam", result->GetName());
  delete result;
  delete parser;
}

// Loads nested entities
void XMLTest::NestedTest() {
  Diadem::Factory factory;

  factory.Register<Diadem::Entity>("entity");

  Diadem::Parser *parser = MakeParser(factory);
  Diadem::Entity *result = parser->LoadEntityFromData(
      "<entity name='outside'>"
        "<entity name='inside'/>"
      "</entity>");

  ASSERT_NE((Diadem::Entity*)NULL, result);
  EXPECT_STREQ("outside", result->GetName());
  ASSERT_EQ(1, result->ChildrenCount());
  EXPECT_STREQ("inside", result->ChildAt(0)->GetName());
  delete result;
  delete parser;
}
