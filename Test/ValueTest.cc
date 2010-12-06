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

#include "Diadem/Metrics.h"
#include "Diadem/Value.h"

TEST(ValueTest, TemplateCoerce) {
  Diadem::Value value = Diadem::Size(15, 24);

  ASSERT_TRUE(value.IsValid());
  ASSERT_TRUE(value.IsValueType<Diadem::Size>());

  Diadem::Size s = value.Coerce<Diadem::Size>();

  ASSERT_EQ(15, s.width);
  ASSERT_EQ(24, s.height);
}

TEST(ValueTest, IntToString) {
  Diadem::Value value(1);

  ASSERT_TRUE(value.IsValid());
  ASSERT_EQ(1, value.Coerce<int32_t>());
  ASSERT_STREQ("1", value.Coerce<Diadem::String>().Get());
}

TEST(ValueTest, StringToInt) {
  Diadem::Value value("27");

  ASSERT_TRUE(value.IsValid());
  ASSERT_EQ(27, value.Coerce<int32_t>());
  ASSERT_STREQ("27", value.Coerce<Diadem::String>().Get());
}

TEST(ValueTest, Assign) {
  Diadem::Value value(27), value2, value3;

  EXPECT_EQ(27, value.Coerce<int32_t>());
  EXPECT_FALSE(value2.IsValid());
  value2 = value;
  EXPECT_EQ(27, value2.Coerce<int32_t>());
  value2 = value3;
  EXPECT_FALSE(value2.IsValid());
}
