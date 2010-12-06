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

#include "Diadem/Layout.h"
#include "Diadem/LibXMLParser.h"
#include "Diadem/NativeCocoa.h"
#include "Diadem/Window.h"

// This doesn't use WindowTestBase because it's Cocoa-only
class CocoaTest : public testing::Test {
 public:
  CocoaTest() : windowObject_(NULL) {}

  testing::AssertionResult ReadWindowData(const char *data);

  void SetUp() {
    pool_ = [[NSAutoreleasePool alloc] init];
  }
  void TearDown() {
    delete windowObject_;
    [pool_ drain];
  }

  // We have to expose this because friend status is not inherited
  NSAlert* AlertForMessageData(Diadem::MessageData *message) {
    return Diadem::Cocoa::AlertForMessageData(message);
  }

  NSAutoreleasePool *pool_;
  Diadem::Window *windowObject_;
  Diadem::Entity *windowRoot_;
};

testing::AssertionResult CocoaTest::ReadWindowData(const char *data) {
  Diadem::Factory factory;

  Diadem::Cocoa::SetUpFactory(&factory);

  Diadem::LibXMLParser parser(factory);

  windowRoot_ = parser.LoadEntityFromData(data);
  if (windowRoot_ == NULL)
    return testing::AssertionFailure() << "load failed";
  if (windowRoot_->GetNative() == NULL)
    return testing::AssertionFailure() << "null native object";
  if (windowRoot_->GetNative()->GetWindowInterface() == NULL)
    return testing::AssertionFailure() << "null window interface";
  windowObject_ = new Diadem::Window(windowRoot_);
  if (windowObject_->ShowModeless())
    windowRoot_->SetProperty(Diadem::kPropLocation, Diadem::Location(20,50));
  return testing::AssertionSuccess();
}

TEST_F(CocoaTest, Button) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Button'><button text='Dot'/></window>"));

  ASSERT_EQ(1, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);

  NSButton *button = (NSButton *)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(button == nil);
  ASSERT_TRUE([button isKindOfClass:[NSButton class]]);
  EXPECT_STREQ("Dot",
      [[button title] cStringUsingEncoding:NSUTF8StringEncoding]);
}

TEST_F(CocoaTest, Label) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Label'><label text='Kaput'/></window>"));

  ASSERT_EQ(1, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);

  NSTextField *label = (NSTextField *)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(label == nil);
  ASSERT_TRUE([label isKindOfClass:[NSTextField class]]);
  EXPECT_STREQ("Kaput",
      [[label stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
}

TEST_F(CocoaTest, EditPassword) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Label'>"
        "<edit text='Your Name'/>"
        "<password/>"
      "</window>"));

  ASSERT_EQ(2, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);
  ASSERT_FALSE(windowRoot_->ChildAt(1)->GetNative() == NULL);

  NSTextField *edit = (NSTextField *)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();
  NSSecureTextField *password = (NSSecureTextField *)
      windowRoot_->ChildAt(1)->GetNative()->GetNativeRef();

  ASSERT_FALSE(edit == nil);
  ASSERT_TRUE([edit isKindOfClass:[NSTextField class]]);
  EXPECT_STREQ("Your Name",
      [[edit stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
  ASSERT_TRUE([password isKindOfClass:[NSSecureTextField class]]);
}

TEST_F(CocoaTest, Popup) {
  ASSERT_TRUE(ReadWindowData(
    "<window>"
      "<popup>"
        "<item text='A'/>"
        "<item text='Beautician'/>"
      "</popup>"
    "</window>"));

  ASSERT_EQ(1, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);

  NSPopUpButton *popup = (NSPopUpButton *)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(popup == nil);
  ASSERT_TRUE([popup isKindOfClass:[NSPopUpButton class]]);
  ASSERT_EQ(2, [popup numberOfItems]);
  EXPECT_STREQ("A", [[popup itemTitleAtIndex:0]
      cStringUsingEncoding:NSUTF8StringEncoding]);
  EXPECT_STREQ("Beautician", [[popup itemTitleAtIndex:1]
      cStringUsingEncoding:NSUTF8StringEncoding]);

  // TODO: check width
}

TEST_F(CocoaTest, Image) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Image'>"
        "<image file='image.png'/>"
      "</window>"));

  ASSERT_EQ(1, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);

  NSImageView *imageView = (NSImageView*)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(imageView == nil);
  ASSERT_TRUE([imageView isKindOfClass:[NSImageView class]]);
  ASSERT_FALSE([imageView image] == nil);

  const NSSize imageSize = [[imageView image] size];

  EXPECT_EQ(289, imageSize.width);
  EXPECT_EQ( 95, imageSize.height);
}

static void ButtonCallback(Diadem::Entity *target, void *data) {
  *(bool *)data = true;
}

TEST_F(CocoaTest, ButtonCallback) {
  ASSERT_TRUE(ReadWindowData("<window><button/></window>"));

  ASSERT_EQ(1, windowRoot_->ChildrenCount());
  ASSERT_FALSE(windowRoot_->ChildAt(0)->GetNative() == NULL);

  NSButton *button = (NSButton *)
      windowRoot_->ChildAt(0)->GetNative()->GetNativeRef();
  bool called = false;

  windowRoot_->SetButtonCallback(ButtonCallback, &called);
  [button performClick:nil];
  EXPECT_TRUE(called);
}

TEST_F(CocoaTest, Enabled) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Enabled'>"
        "<button name='b' text='Button'/>"
        "<popup name='p'>"
          "<item text='Item'/>"
        "</popup>"
      "</window>"));

  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity *button = windowRoot_->FindByName("b");
  Diadem::Entity *popup  = windowRoot_->FindByName("p");

  ASSERT_FALSE(button == NULL);
  ASSERT_FALSE(popup == NULL);
  ASSERT_EQ(1, popup->ChildrenCount());

  Diadem::Entity *item = popup->ChildAt(0);

  ASSERT_FALSE(item == NULL);

  EXPECT_TRUE(button->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());
  EXPECT_TRUE(popup->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());
  EXPECT_TRUE(item->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());

  EXPECT_TRUE(button->SetProperty(Diadem::Entity::kPropEnabled, false));
  EXPECT_FALSE([(id)button->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(button->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());

  EXPECT_TRUE(popup->SetProperty(Diadem::Entity::kPropEnabled, false));
  EXPECT_FALSE([(id)popup->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(popup->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());

  EXPECT_TRUE(item->SetProperty(Diadem::Entity::kPropEnabled, false));
  EXPECT_FALSE([(id)item->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(item->GetProperty(
      Diadem::Entity::kPropEnabled).Coerce<Diadem::Bool>());
}

TEST_F(CocoaTest, MessagePlain) {
  Diadem::MessageData message("Something happened!");
  NSAlert *alert = AlertForMessageData(&message);

  ASSERT_FALSE(alert == nil);
  [alert layout];
  EXPECT_EQ(1, [[alert buttons] count]);
  EXPECT_STREQ("Something happened!", [[alert messageText] UTF8String]);
}

TEST_F(CocoaTest, MessageCancel) {
  Diadem::MessageData message("Let's go to the zoo.");

  message.show_cancel_ = true;

  NSAlert *alert = AlertForMessageData(&message);

  ASSERT_FALSE(alert == nil);
  EXPECT_EQ(2, [[alert buttons] count]);
  EXPECT_STREQ(
      "Cancel",
      [[[[alert buttons] objectAtIndex:1] title] UTF8String]);
  EXPECT_STREQ("Let's go to the zoo.", [[alert messageText] UTF8String]);
}

TEST_F(CocoaTest, MessageSuppress) {
  Diadem::MessageData message("Poke!");

  message.show_suppress_ = true;
  message.suppress_text_ = "Stop it";

  NSAlert *alert = AlertForMessageData(&message);

  ASSERT_FALSE(alert == nil);
  EXPECT_TRUE([alert showsSuppressionButton]);
  EXPECT_STREQ(
      "Stop it",
      [[[alert suppressionButton] title] UTF8String]);
}
