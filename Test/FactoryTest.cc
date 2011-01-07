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

#include "Diadem/Factory.h"
#include "Diadem/Value.h"

class FactoryTest : public testing::Test {
 protected:
  virtual void SetUp() {}

  virtual void TearDown() {}
};

class TestEntity : public Diadem::Entity {
 public:
  TestEntity()
      : didFinalize_(false) {}

  Diadem::Bool DidFinalize() const { return didFinalize_; }

 protected:
  Diadem::Bool didFinalize_;

  void Finalize() { didFinalize_ = true; }
};

#define kEntityClassName "entity"

// Constructs a plain, named entity
TEST(FactoryTest, ConstructEntity) {
  Diadem::Factory factory;
  Diadem::PropertyMap properties;

  factory.Register<Diadem::Entity>(kEntityClassName);
  properties.Insert(Diadem::Entity::kPropName, "Flauze");

  Diadem::Entity *entity = factory.CreateEntity(kEntityClassName, properties);

  ASSERT_NE(entity, (Diadem::Entity*)NULL);
  EXPECT_STREQ("Flauze", entity->GetName());
  delete entity;
}

// Directly exercises FactorySession, normally only called by the Parser
TEST(FactoryTest, BasicSession) {
  Diadem::Factory factory;
  Diadem::FactorySession session(factory);
  Diadem::PropertyMap properties;

  factory.Register<TestEntity>(kEntityClassName);
  properties.Insert(Diadem::Entity::kPropName, "Parent");
  session.BeginEntity(kEntityClassName, properties);
  EXPECT_NE((Diadem::Entity*)NULL, session.RootEntity());
  EXPECT_EQ(session.RootEntity(), session.CurrentEntity());
  EXPECT_STREQ("Parent", session.RootEntity()->GetName());

  TestEntity* const parent = dynamic_cast<TestEntity*>(session.RootEntity());
  ASSERT_NE((TestEntity*)NULL, parent);

  properties[Diadem::Entity::kPropName] = Diadem::String("Child");
  session.BeginEntity(kEntityClassName, properties);
  EXPECT_NE((Diadem::Entity*)NULL, session.CurrentEntity());
  EXPECT_NE(session.RootEntity(), session.CurrentEntity());
  EXPECT_STREQ("Child", session.CurrentEntity()->GetName());

  TestEntity* const child = dynamic_cast<TestEntity*>(session.CurrentEntity());
  ASSERT_NE((TestEntity*)NULL, child);

  EXPECT_FALSE(parent->DidFinalize());
  EXPECT_FALSE(child->DidFinalize());

  session.EndEntity();
  session.EndEntity();

  EXPECT_TRUE(parent->DidFinalize());
  EXPECT_TRUE(child->DidFinalize());
}
