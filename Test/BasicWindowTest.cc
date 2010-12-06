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

class BasicWindowTest : public WindowTestBase {
};

TEST_F(BasicWindowTest, SimpleWindow) {
  ReadWindowData(
      "<window text='SimpleWindow'>"
        "<button text='Death by Monkeys'/>"
      "</window>");

  EXPECT_STREQ(
      "SimpleWindow",
      windowRoot_->GetProperty(Entity::kPropText).Coerce<String>());
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const button = windowRoot_->ChildAt(0);

  ASSERT_FALSE(button == NULL);
  EXPECT_STREQ(
      "Death by Monkeys",
      button->GetProperty(Entity::kPropText).Coerce<String>());
}

TEST_F(BasicWindowTest, EditField) {
  ReadWindowData(
      "<window text='EditField'>"
        "<edit text='platypus'/>"
      "</window>");
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const edit = windowRoot_->ChildAt(0);

  EXPECT_STREQ(
      "platypus",
      edit->GetProperty(Entity::kPropText).Coerce<String>());
  EXPECT_TRUE(edit->SetProperty(Entity::kPropText, "skunk"));
  EXPECT_STREQ(
      "skunk",
      edit->GetProperty(Entity::kPropText).Coerce<String>());
}

TEST_F(BasicWindowTest, PasswordField) {
  ReadWindowData(
      "<window text='PasswordField'>"
        "<password text='armadillo'/>"
      "</window>");
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Entity* const edit = windowRoot_->ChildAt(0);

  EXPECT_STREQ(
      "armadillo",
      edit->GetProperty(Entity::kPropText).Coerce<String>());
  EXPECT_TRUE(edit->SetProperty(Entity::kPropText, "gopher"));
  EXPECT_STREQ(
      "gopher",
      edit->GetProperty(Entity::kPropText).Coerce<String>());
}
