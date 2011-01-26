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
#include "Diadem/Value.h"
#include "Diadem/Wrappers.h"

using Diadem::Entity;
using Diadem::String;

// Simple tests of window creation and control attributes.
class BasicWindowTest : public WindowTestBase {
};

// Verify window and button titles
TEST_F(BasicWindowTest, SimpleWindow) {
  ReadWindowData(
      "<window text='SimpleWindow'>"
        "<button text='Death by Monkeys'/>"
      "</window>");

  EXPECT_STREQ(
      "SimpleWindow",
      windowRoot_->GetProperty(Diadem::kPropText).Coerce<String>());
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const button = windowRoot_->ChildAt(0);

  ASSERT_FALSE(button == NULL);
  EXPECT_STREQ(
      "Death by Monkeys",
      button->GetProperty(Diadem::kPropText).Coerce<String>());
}

// Change the contents of an edit field.
TEST_F(BasicWindowTest, EditField) {
  ReadWindowData(
      "<window text='EditField'>"
        "<edit text='platypus'/>"
      "</window>");
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const edit = windowRoot_->ChildAt(0);

  EXPECT_STREQ(
      "platypus",
      edit->GetProperty(Diadem::kPropText).Coerce<String>());
  EXPECT_TRUE(edit->SetProperty(Diadem::kPropText, "skunk"));
  EXPECT_STREQ(
      "skunk",
      edit->GetProperty(Diadem::kPropText).Coerce<String>());
}

// Change the contents of a password field.
TEST_F(BasicWindowTest, PasswordField) {
  ReadWindowData(
      "<window text='PasswordField'>"
        "<password text='armadillo'/>"
      "</window>");
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const edit = windowRoot_->ChildAt(0);

  EXPECT_STREQ(
      "armadillo",
      edit->GetProperty(Diadem::kPropText).Coerce<String>());
  EXPECT_TRUE(edit->SetProperty(Diadem::kPropText, "gopher"));
  EXPECT_STREQ(
      "gopher",
      edit->GetProperty(Diadem::kPropText).Coerce<String>());
}

// Tests entity type names
TEST_F(BasicWindowTest, TypeNames) {
  ReadWindowData(
      "<window text='TypeNames'>"
        "<button text='Button'/>"
        "<group><edit/><password/></group>"
      "</window>");
  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  EXPECT_STREQ("window", windowRoot_->GetTypeName());

  Diadem::Entity* const button   = windowRoot_->ChildAt(0);
  Diadem::Entity* const group    = windowRoot_->ChildAt(1);

  ASSERT_EQ(2, group->ChildrenCount());

  Diadem::Entity* const edit     = group->ChildAt(0);
  Diadem::Entity* const password = group->ChildAt(1);

  EXPECT_STREQ("button", button->GetTypeName());
  EXPECT_STREQ("group", group->GetTypeName());
  EXPECT_STREQ("edit", edit->GetTypeName());
  EXPECT_STREQ("password", password->GetTypeName());
}

// Tests entity paths
TEST_F(BasicWindowTest, Paths) {
  ReadWindowData(
      "<window text='Paths'>"
        "<label text='a'/>"
        "<group name='g'>"
          "<label text='b'/>"
        "</group>"
        "<group>"
          "<label text='c'/>"
        "</group>"
        "<label text='d'/>"
      "</window>");
  ASSERT_EQ(4, windowRoot_->ChildrenCount());

  Diadem::Entity* const labelA = windowRoot_->ChildAt(0);
  Diadem::Entity* const group1 = windowRoot_->ChildAt(1);
  Diadem::Entity* const labelB = group1->ChildAt(0);
  Diadem::Entity* const group2 = windowRoot_->ChildAt(2);
  Diadem::Entity* const labelC = group2->ChildAt(0);
  Diadem::Entity* const labelD = windowRoot_->ChildAt(3);

  EXPECT_EQ(1, windowRoot_->ChildIndexByType(labelA));
  EXPECT_EQ(1, group1->ChildIndexByType(labelB));
  EXPECT_EQ(1, group2->ChildIndexByType(labelC));
  EXPECT_EQ(2, windowRoot_->ChildIndexByType(labelD));
  EXPECT_EQ(1, windowRoot_->ChildIndexByType(group1));
  EXPECT_EQ(2, windowRoot_->ChildIndexByType(group2));

  EXPECT_STREQ("/window", windowRoot_->GetPath());
  EXPECT_STREQ("/window/label1", labelA->GetPath());
  EXPECT_STREQ("\"g\"", group1->GetPath());
  EXPECT_STREQ("\"g\"/label1", labelB->GetPath());
  EXPECT_STREQ("/window/group2", group2->GetPath());
  EXPECT_STREQ("/window/group2/label1", labelC->GetPath());
  EXPECT_STREQ("/window/label2", labelD->GetPath());
}

// Tests parsing the window "style" attribute
TEST_F(BasicWindowTest, ParseStyle) {
  uint32_t result = Diadem::Native::ParseWindowStyle("close");

  EXPECT_EQ(Diadem::kStyleClosable, result);

  result = Diadem::Native::ParseWindowStyle("close,size");
  EXPECT_EQ(Diadem::kStyleClosable | Diadem::kStyleResizable, result);

  result = Diadem::Native::ParseWindowStyle("min");
  EXPECT_EQ(Diadem::kStyleMinimizable, result);
}
