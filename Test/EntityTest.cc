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

#include "Diadem/Entity.h"
#include "Diadem/Value.h"
#include "Diadem/Wrappers.h"

class EntityTest : public testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void TearDown() {}
};

// Tests FindByName, which should match the entity itself or a child
TEST(EntityTest, FindByName) {
  Diadem::Entity parent, child1, child2;

  parent.SetName("papa");
  child1.SetName("bill");
  child2.SetName("ted");
  parent.AddChild(&child1);
  parent.AddChild(&child2);
  EXPECT_EQ(parent.FindByName("papa"), &parent);
  EXPECT_EQ(parent.FindByName("bill"), &child1);
  EXPECT_EQ(parent.FindByName("ted"), &child2);
  parent.RemoveChild(&child1);
  parent.RemoveChild(&child2);
}

// Constructs with a PropertyMap specifying a name
TEST(EntityTest, ConstructWithName) {
  Diadem::PropertyMap properties;

  properties.Insert(Diadem::Entity::kPropName, "Courage");

  Diadem::Entity entity;
  entity.InitializeProperties(properties);

  EXPECT_STREQ("Courage", entity.GetName());
  entity.SetProperty(Diadem::Entity::kPropName, Diadem::Value("Eustace"));
  EXPECT_STREQ("Eustace", entity.GetName());
}

// Tests parent-child relationships
TEST(EntityTest, ParentChild) {
  Diadem::PropertyMap properties;
  Diadem::Entity parent, child1, child2;

  parent.InitializeProperties(properties);
  child1.InitializeProperties(properties);
  child2.InitializeProperties(properties);

  EXPECT_EQ(0, parent.ChildrenCount());
  EXPECT_EQ((Diadem::Entity*)NULL, child1.GetParent());
  parent.AddChild(&child1);
  EXPECT_EQ(1, parent.ChildrenCount());
  EXPECT_EQ(&parent, child1.GetParent());
  EXPECT_EQ(&child1, parent.ChildAt(0));
  parent.AddChild(&child2);
  EXPECT_EQ(2, parent.ChildrenCount());
  EXPECT_EQ(&parent, child2.GetParent());
  EXPECT_EQ(&child2, parent.ChildAt(1));
  parent.RemoveChild(&child1);
  EXPECT_EQ(1, parent.ChildrenCount());
  EXPECT_EQ((Diadem::Entity*)NULL, child1.GetParent());
  EXPECT_EQ(&child2, parent.ChildAt(0));
  parent.RemoveChild(&child2);
}
