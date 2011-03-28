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
#include "Diadem/Binding.h"
#include "Diadem/Value.h"

class TestBindings : public WindowTestBase {};

TEST_F(TestBindings, testRemoveObserver) {
  class Ob : public Diadem::ValueObserver {
   public:
    Ob() : observe_count_(0) {}

    void ObserveImp(const char*, const Diadem::Value&)
      { ++observe_count_; }

    unsigned int observe_count_;
  };

  Diadem::ChangeMessenger messenger;
  Ob ob1, ob2;
  Diadem::Value v;

  EXPECT_EQ(0, ob1.observe_count_);
  EXPECT_EQ(0, ob2.observe_count_);
  messenger.AddObserver("", &ob1);
  messenger.AddObserver("a", &ob2);
  messenger.NotifyChange("", v);
  EXPECT_EQ(1, ob1.observe_count_);
  EXPECT_EQ(0, ob2.observe_count_);
  messenger.NotifyChange("a", v);
  EXPECT_EQ(2, ob1.observe_count_);
  EXPECT_EQ(1, ob2.observe_count_);
  messenger.RemoveObserver(&ob1);
  messenger.NotifyChange("", v);
  EXPECT_EQ(2, ob1.observe_count_);
  EXPECT_EQ(1, ob2.observe_count_);
  messenger.NotifyChange("a", v);
  EXPECT_EQ(2, ob1.observe_count_);
  EXPECT_EQ(2, ob2.observe_count_);
  messenger.RemoveObserver(&ob2);
  messenger.NotifyChange("a", v);
  EXPECT_EQ(2, ob1.observe_count_);
  EXPECT_EQ(2, ob2.observe_count_);
}

TEST_F(TestBindings, testValueToEnable) {
  ReadWindowData(
      "<window text='testValueToEnable'>"
        "<label text='Label' name='label'>"
          "<bind source='check' prop='enabled'/>"
        "</label>"
        "<check title='Check' name='check'/>"
      "</window>");
  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity *label = windowRoot_->FindByName("label");
  Diadem::Entity *checkbox = windowRoot_->FindByName("check");
  ASSERT_EQ(1, label->ChildrenCount());
  Diadem::Binding *binding = dynamic_cast<Diadem::Binding*>(label->ChildAt(0));
  ASSERT_FALSE(binding == NULL);

  const Diadem::EntityController &controller = binding->GetController();

  EXPECT_EQ(label, controller.GetEntity());
  EXPECT_STREQ("enabled", controller.GetPropertyName());

  EXPECT_EQ(0, checkbox->GetProperty(Diadem::kPropValue).Coerce<int32_t>());
  EXPECT_FALSE(label->GetProperty(Diadem::kPropEnabled).Coerce<bool>());

  checkbox->SetProperty(Diadem::kPropValue, 1);

  EXPECT_TRUE(label->GetProperty(Diadem::kPropEnabled).Coerce<bool>());
}
