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

#include "Diadem/LabelGroup.h"
#include "Diadem/Layout.h"
#include "Diadem/LibXMLParser.h"
#include "Diadem/NativeCocoa.h"
#include "Diadem/Window.h"

// This doesn't use WindowTestBase because it's Cocoa-only
class CocoaTest : public testing::Test {
 public:
  CocoaTest() : window_object_(NULL) {}

  testing::AssertionResult ReadWindowData(const char *data);

  void SetUp() {
    pool_ = [[NSAutoreleasePool alloc] init];
  }
  void TearDown() {
    delete window_object_;
    [pool_ drain];
  }

  // We have to expose this because friend status is not inherited
  NSAlert* AlertForMessageData(Diadem::MessageData *message) {
    return Diadem::Cocoa::AlertForMessageData(message);
  }

  NSAutoreleasePool *pool_;
  Diadem::Window *window_object_;
  Diadem::Entity *window_root_;
};

testing::AssertionResult CocoaTest::ReadWindowData(const char *data) {
  Diadem::Factory factory;

  Diadem::Cocoa::SetUpFactory(&factory);

  Diadem::LibXMLParser parser(factory);

  window_root_ = parser.LoadEntityFromData(data);
  if (window_root_ == NULL)
    return testing::AssertionFailure() << "load failed";
  if (window_root_->GetNative() == NULL)
    return testing::AssertionFailure() << "null native object";
  if (window_root_->GetNative()->GetWindowInterface() == NULL)
    return testing::AssertionFailure() << "null window interface";
  window_object_ = new Diadem::Window(window_root_);
  if (window_object_->ShowModeless())
    window_root_->SetProperty(Diadem::kPropLocation, Diadem::Location(20,50));
  return testing::AssertionSuccess();
}

// Verifies button control and title
TEST_F(CocoaTest, Button) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Button'><button text='Dot'/></window>"));

  ASSERT_EQ(1, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);

  NSButton *button = (NSButton *)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(button == nil);
  ASSERT_TRUE([button isKindOfClass:[NSButton class]]);
  EXPECT_STREQ("Dot",
      [[button title] cStringUsingEncoding:NSUTF8StringEncoding]);
}

// Verifies label (NSTextField) control and title
TEST_F(CocoaTest, Label) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Label'><label text='Kaput'/></window>"));

  ASSERT_EQ(1, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);

  NSTextField *label = (NSTextField *)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(label == nil);
  ASSERT_TRUE([label isKindOfClass:[NSTextField class]]);
  EXPECT_STREQ("Kaput",
      [[label stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
}

// Verifies edit field and password variation
TEST_F(CocoaTest, EditPassword) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Label'>"
        "<edit text='Your Name'/>"
        "<password/>"
      "</window>"));

  ASSERT_EQ(2, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);
  ASSERT_FALSE(window_root_->ChildAt(1)->GetNative() == NULL);

  NSTextField *edit = (NSTextField *)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();
  NSSecureTextField *password = (NSSecureTextField *)
      window_root_->ChildAt(1)->GetNative()->GetNativeRef();

  ASSERT_FALSE(edit == nil);
  ASSERT_TRUE([edit isKindOfClass:[NSTextField class]]);
  EXPECT_STREQ("Your Name",
      [[edit stringValue] cStringUsingEncoding:NSUTF8StringEncoding]);
  ASSERT_TRUE([password isKindOfClass:[NSSecureTextField class]]);
}

// Verifies popup button and menu contents
TEST_F(CocoaTest, Popup) {
  ASSERT_TRUE(ReadWindowData(
    "<window>"
      "<popup>"
        "<item text='A'/>"
        "<item text='Beautician'/>"
      "</popup>"
    "</window>"));

  ASSERT_EQ(1, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);

  NSPopUpButton *popup = (NSPopUpButton *)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(popup == nil);
  ASSERT_TRUE([popup isKindOfClass:[NSPopUpButton class]]);
  ASSERT_EQ(2, [popup numberOfItems]);
  EXPECT_STREQ("A", [[popup itemTitleAtIndex:0]
      cStringUsingEncoding:NSUTF8StringEncoding]);
  EXPECT_STREQ("Beautician", [[popup itemTitleAtIndex:1]
      cStringUsingEncoding:NSUTF8StringEncoding]);

  // TODO: check width
}

// Verifies image loaded from a file
TEST_F(CocoaTest, Image) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Image'>"
        "<image file='image.png'/>"
      "</window>"));

  ASSERT_EQ(1, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);

  NSImageView *image_view = (NSImageView*)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();

  ASSERT_FALSE(image_view == nil);
  ASSERT_TRUE([image_view isKindOfClass:[NSImageView class]]);
  ASSERT_FALSE([image_view image] == nil);

  const NSSize image_size = [[image_view image] size];

  EXPECT_EQ(289, image_size.width);
  EXPECT_EQ( 95, image_size.height);
}

// Verifies box and its metrics
TEST_F(CocoaTest, Box) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testBox'>"
        "<box name='b'>"
          "<label text='Help!' name='h'/>"
        "</box>"
      "</window>"));

  Diadem::Entity* const box = window_root_->FindByName("b");
  Diadem::Entity* const label = window_root_->FindByName("h");
  ASSERT_FALSE(box == NULL);
  ASSERT_FALSE(label == NULL);

  NSBox *box_view = (NSBox*)box->GetNative()->GetNativeRef();
  NSView *label_view = (NSView*)label->GetNative()->GetNativeRef();

  EXPECT_EQ([label_view superview], [box_view contentView]);

  const NSRect label_frame = [label_view frame];
  const NSRect box_rect = [[box_view contentView] bounds];
  const Diadem::Spacing box_margins =
      box->GetProperty(Diadem::kPropMargins).Coerce<Diadem::Spacing>();

  EXPECT_EQ(box_margins.left, label_frame.origin.x + 1);  // 1 for inset
  EXPECT_EQ(box_margins.top,
      box_rect.size.height - label_frame.origin.y - label_frame.size.height);
}

static void ButtonCallback(Diadem::Entity *target, void *data) {
  *(bool *)data = true;
}

// Tests that the button callback works
TEST_F(CocoaTest, ButtonCallback) {
  ASSERT_TRUE(ReadWindowData("<window><button/></window>"));

  ASSERT_EQ(1, window_root_->ChildrenCount());
  ASSERT_FALSE(window_root_->ChildAt(0)->GetNative() == NULL);

  NSButton *button = (NSButton *)
      window_root_->ChildAt(0)->GetNative()->GetNativeRef();
  bool called = false;

  window_root_->SetButtonCallback(ButtonCallback, &called);
  [button performClick:nil];
  EXPECT_TRUE(called);
}

// Tests enabling and disabling
TEST_F(CocoaTest, Enabled) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Enabled'>"
        "<button name='b' text='Button'/>"
        "<popup name='p'>"
          "<item text='Item'/>"
        "</popup>"
      "</window>"));

  ASSERT_EQ(2, window_root_->ChildrenCount());

  Diadem::Entity *button = window_root_->FindByName("b");
  Diadem::Entity *popup  = window_root_->FindByName("p");

  ASSERT_FALSE(button == NULL);
  ASSERT_FALSE(popup == NULL);
  ASSERT_EQ(1, popup->ChildrenCount());

  Diadem::Entity *item = popup->ChildAt(0);

  ASSERT_FALSE(item == NULL);

  EXPECT_TRUE(button->GetProperty(Diadem::kPropEnabled).Coerce<bool>());
  EXPECT_TRUE(popup->GetProperty(Diadem::kPropEnabled).Coerce<bool>());
  EXPECT_TRUE(item->GetProperty(Diadem::kPropEnabled).Coerce<bool>());

  EXPECT_TRUE(button->SetProperty(Diadem::kPropEnabled, false));
  EXPECT_FALSE([(id)button->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(button->GetProperty(
      Diadem::kPropEnabled).Coerce<bool>());

  EXPECT_TRUE(popup->SetProperty(Diadem::kPropEnabled, false));
  EXPECT_FALSE([(id)popup->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(popup->GetProperty(Diadem::kPropEnabled).Coerce<bool>());

  EXPECT_TRUE(item->SetProperty(Diadem::kPropEnabled, false));
  EXPECT_FALSE([(id)item->GetNative()->GetNativeRef() isEnabled]);
  EXPECT_FALSE(item->GetProperty(Diadem::kPropEnabled).Coerce<bool>());
}

// Simple case for ShowMessage - message with OK buton
TEST_F(CocoaTest, MessagePlain) {
  Diadem::MessageData message("Something happened!");
  NSAlert *alert = AlertForMessageData(&message);

  ASSERT_FALSE(alert == nil);
  [alert layout];
  EXPECT_EQ(1, [[alert buttons] count]);
  EXPECT_STREQ("Something happened!", [[alert messageText] UTF8String]);
}

// ShowMessage with OK and Cancel buttons
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

// ShowMessage with suppression checkbox visible
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

// Makes sure a labelgroup's children are added to the window's content view.
TEST_F(CocoaTest, testColumnLabelGroupSimple) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testColumnLabelGroupSimple'>"
        "<labelgroup text='Exit:'>"
          "<label text='Stage left'/>"
        "</labelgroup>"
      "</window>"));

  Diadem::LabelGroup *labelgroup =
      dynamic_cast<Diadem::LabelGroup*>(window_root_->ChildAt(0));
  ASSERT_FALSE(labelgroup == NULL);

  Diadem::Entity *group_label = labelgroup->GetLabel();
  NSView *label_view = (NSView*)group_label->GetNative()->GetNativeRef();
  NSWindow *native_window =
      (NSWindow*)window_root_->GetNative()->GetNativeRef();

  EXPECT_EQ(native_window, [label_view window]);
  EXPECT_EQ(
      (NSTextAlignment)NSRightTextAlignment,
      [(NSTextField*)label_view alignment]);

  Diadem::Entity *content_label = labelgroup->GetContent()->ChildAt(0);

  label_view = (NSView*)content_label->GetNative()->GetNativeRef();
  EXPECT_EQ(native_window, [label_view window]);
}

TEST_F(CocoaTest, testWindowStyle1) {
  ASSERT_TRUE(ReadWindowData(
    "<window text='testWindowStyle1' style='close'/>"));

  NSWindow *window = (NSWindow*)window_root_->GetNative()->GetNativeRef();

  EXPECT_TRUE([window styleMask] & NSClosableWindowMask);
  EXPECT_FALSE([window styleMask] & NSResizableWindowMask);
}

TEST_F(CocoaTest, testWindowStyle2) {
  ASSERT_TRUE(ReadWindowData(
    "<window text='testWindowStyle2' style='close,size'/>"));

  NSWindow *window = (NSWindow*)window_root_->GetNative()->GetNativeRef();

  EXPECT_TRUE([window styleMask] & NSClosableWindowMask);
  EXPECT_TRUE([window styleMask] & NSResizableWindowMask);
}

TEST_F(CocoaTest, testWindowStyle3) {
  ASSERT_TRUE(ReadWindowData(
    "<window text='testWindowStyle3'/>"));

  NSWindow *window = (NSWindow*)window_root_->GetNative()->GetNativeRef();

  EXPECT_FALSE([window styleMask] & NSClosableWindowMask);
  EXPECT_FALSE([window styleMask] & NSResizableWindowMask);
}

TEST_F(CocoaTest, testCheckbox) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='CheckboxValue'>"
        "<check text='Check'/>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const checkbox = window_root_->ChildAt(0);
  NSButton *button = reinterpret_cast<NSButton*>(
      checkbox->GetNative()->GetNativeRef());

  ASSERT_TRUE([button isKindOfClass:[NSButton class]]);
  EXPECT_EQ((NSCellStateValue)NSOffState, [button state]);
  EXPECT_TRUE(checkbox->SetProperty(Diadem::kPropValue, 1));
  EXPECT_EQ((NSCellStateValue)NSOnState, [button state]);
  [button setState:NSOffState];
  EXPECT_EQ(0, checkbox->GetProperty(Diadem::kPropValue).Coerce<int32_t>());
  EXPECT_FALSE([button target] == nil);
}

TEST_F(CocoaTest, testLabelStyle) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testLabelStyle'>"
        "<label text='Normal'/>"
        "<label text='Bold' style='head'/>"
        "<label text='Small' uisize='small'/>"
        "<label text='Small bold' uisize='small' style='head'/>"
        "<label text='Small bold' style='head' uisize='small'/>"
      "</window>"));

  Diadem::Entity *label = window_root_->ChildAt(0);
  NSTextField *text = (NSTextField*)label->GetNative()->GetNativeRef();
  NSFont *font = [text font];
  NSFont *expected_font = [NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSRegularControlSize]];

  EXPECT_TRUE([font isEqual:expected_font]);

  label = window_root_->ChildAt(1);
  text = (NSTextField*)label->GetNative()->GetNativeRef();
  font = [text font];
  expected_font = [NSFont boldSystemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSRegularControlSize]];
  EXPECT_TRUE([font isEqual:expected_font]);

  label = window_root_->ChildAt(2);
  text = (NSTextField*)label->GetNative()->GetNativeRef();
  font = [text font];
  expected_font = [NSFont systemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSSmallControlSize]];
  EXPECT_TRUE([font isEqual:expected_font]);

  label = window_root_->ChildAt(3);
  text = (NSTextField*)label->GetNative()->GetNativeRef();
  font = [text font];
  expected_font = [NSFont boldSystemFontOfSize:
      [NSFont systemFontSizeForControlSize:NSSmallControlSize]];
  EXPECT_TRUE([font isEqual:expected_font]);

  label = window_root_->ChildAt(4);
  text = (NSTextField*)label->GetNative()->GetNativeRef();
  font = [text font];
  // same expected font
  EXPECT_TRUE([font isEqual:expected_font]);
}

TEST_F(CocoaTest, testBasicList) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testBasicList'>"
        "<list>"
          "<column name='col1' text='Work'/>"
          "<column name='col2' text='Play'/>"
        "</list>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const list = window_root_->ChildAt(0);
  NSScrollView *scroll_view =
      reinterpret_cast<NSScrollView*>(list->GetNative()->GetNativeRef());

  ASSERT_TRUE([scroll_view isKindOfClass:[NSScrollView class]]);

  NSTableView *table_view =
      reinterpret_cast<NSTableView*>([scroll_view documentView]);

  ASSERT_TRUE([table_view isKindOfClass:[NSTableView class]]);
  ASSERT_EQ(2, list->ChildrenCount());
  ASSERT_EQ(2, [[table_view tableColumns] count]);

  NSTableColumn *col = [[table_view tableColumns] objectAtIndex:0];

  EXPECT_STREQ("col1", [[col identifier] UTF8String]);
  EXPECT_STREQ("Work", [[[col headerCell] stringValue] UTF8String]);

  col = [[table_view tableColumns] objectAtIndex:1];
  EXPECT_STREQ("col2", [[col identifier] UTF8String]);
  EXPECT_STREQ("Play", [[[col headerCell] stringValue] UTF8String]);
}

TEST_F(CocoaTest, testColumnSizes) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testBasicList'>"
        "<list>"
          "<column name='col1' width='40'/>"
          "<column name='col2' width='5em'/>"
        "</list>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const list = window_root_->ChildAt(0);
  NSScrollView *scroll_view =
      reinterpret_cast<NSScrollView*>(list->GetNative()->GetNativeRef());

  ASSERT_TRUE([scroll_view isKindOfClass:[NSScrollView class]]);

  NSTableView *table_view =
      reinterpret_cast<NSTableView*>([scroll_view documentView]);

  ASSERT_TRUE([table_view isKindOfClass:[NSTableView class]]);
  ASSERT_EQ(2, list->ChildrenCount());
  ASSERT_EQ(2, [[table_view tableColumns] count]);

  Diadem::Entity *col = list->ChildAt(0);
  Diadem::Size size =
      col->GetProperty(Diadem::kPropMinimumSize).Coerce<Diadem::Size>();

  EXPECT_EQ(40, size.width);

  col = list->ChildAt(1);
  size = col->GetProperty(Diadem::kPropMinimumSize).Coerce<Diadem::Size>();
  EXPECT_EQ(col->GetNative()->GetPlatformMetrics().em_size * 5, size.width);
}

TEST_F(CocoaTest, testSlider) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testSlider'>"
        "<slider min='1' max='100' value='20' ticks='11'/>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const slider = window_root_->ChildAt(0);
  id slider_ref = (id)slider->GetNative()->GetNativeRef();

  ASSERT_FALSE(slider_ref == nil);
  EXPECT_TRUE([slider_ref isKindOfClass:[NSSlider class]]);
  EXPECT_EQ(1, slider->GetProperty(Diadem::kPropMin).Coerce<int32_t>());
  EXPECT_EQ(100, slider->GetProperty(Diadem::kPropMax).Coerce<int32_t>());
  EXPECT_EQ(20, slider->GetProperty(Diadem::kPropValue).Coerce<int32_t>());
  EXPECT_EQ(11, slider->GetProperty(Diadem::kPropTicks).Coerce<int32_t>());
}

TEST_F(CocoaTest, testSliderFloat) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testSlider'>"
        "<slider min='0.5' max='1.5' value='0.75'/>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const slider = window_root_->ChildAt(0);
  id slider_ref = (id)slider->GetNative()->GetNativeRef();

  ASSERT_FALSE(slider_ref == nil);
  EXPECT_TRUE([slider_ref isKindOfClass:[NSSlider class]]);
  EXPECT_EQ(0.5, slider->GetProperty(Diadem::kPropMin).Coerce<double>());
  EXPECT_EQ(1.5, slider->GetProperty(Diadem::kPropMax).Coerce<double>());
  EXPECT_EQ(0.75, slider->GetProperty(Diadem::kPropValue).Coerce<double>());
}

TEST_F(CocoaTest, testRadio) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testRadio'>"
        "<radiogroup>"
          "<radio text='A'/>"
          "<radio text='B'/>"
        "</radiogroup>"
      "</window>"));
  ASSERT_EQ(1, window_root_->ChildrenCount());

  Diadem::Entity* const group = window_root_->ChildAt(0);
  ASSERT_EQ(2, group->ChildrenCount());
  Diadem::Entity* const radio1 = group->ChildAt(0);
  Diadem::Entity* const radio2 = group->ChildAt(1);

  ASSERT_FALSE(radio1->GetNative() == NULL);
  ASSERT_FALSE(radio2->GetNative() == NULL);
  NSButton *button1 =
      reinterpret_cast<NSButton*>(radio1->GetNative()->GetNativeRef());
  NSButton *button2 =
      reinterpret_cast<NSButton*>(radio2->GetNative()->GetNativeRef());

  EXPECT_EQ(0, [button1 bezelStyle]);
  EXPECT_EQ(static_cast<NSInteger>(NSOnState), [button1 state]);
  EXPECT_EQ(static_cast<NSInteger>(NSOffState), [button2 state]);
  [button2 performClick:nil];
  EXPECT_EQ(static_cast<NSInteger>(NSOffState), [button1 state]);
  EXPECT_EQ(static_cast<NSInteger>(NSOnState), [button2 state]);
  [button1 performClick:nil];
  EXPECT_EQ(static_cast<NSInteger>(NSOnState), [button1 state]);
  EXPECT_EQ(static_cast<NSInteger>(NSOffState), [button2 state]);
}

// Mirrors GroupTest.testLatentVisibility, testing actual control visibility
TEST_F(CocoaTest, testLatentVisibility) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='testLatentVisibility'>"
        "<group>"
          "<label text='Invisible!'/>"
        "</group>"
      "</window>"));

  Diadem::Entity *group = window_root_->ChildAt(0);
  Diadem::Entity *label = group->ChildAt(0);
  NSTextField *label_ref =
      reinterpret_cast<NSTextField*>(label->GetNative()->GetNativeRef());

  EXPECT_FALSE([label_ref isHidden]);

  EXPECT_TRUE(group->SetProperty(Diadem::kPropVisible, false));
  EXPECT_TRUE([label_ref isHidden]);

  EXPECT_TRUE(label->SetProperty(Diadem::kPropVisible, false));
  EXPECT_TRUE([label_ref isHidden]);

  EXPECT_TRUE(group->SetProperty(Diadem::kPropVisible, true));
  EXPECT_TRUE([label_ref isHidden]);

  EXPECT_TRUE(group->SetProperty(Diadem::kPropVisible, false));
  EXPECT_TRUE([label_ref isHidden]);
  EXPECT_TRUE(label->SetProperty(Diadem::kPropVisible, true));
  EXPECT_TRUE([label_ref isHidden]);

  EXPECT_TRUE(group->SetProperty(Diadem::kPropVisible, true));
  EXPECT_FALSE([label_ref isHidden]);
}

// Label groups are special because of their special label handling
TEST_F(CocoaTest, testLabelGroupVisibility) {
  ASSERT_TRUE(ReadWindowData(
      "<window text='Preferences - Google Drive'>"
        "<popup name='p'>"
          "<item text='One'/>"
          "<item text='Two'/>"
        "</popup>"
        "<multi name='multi'>"
          "<bind source='p' prop='value'/>"
          "<label text='One' width='fill' name='account'/>"
          "<labelgroup text='Two:' width='fill'>"
            "<label text='point five' width='fill' name='l2'/>"
          "</labelgroup>"
        "</multi>"
      "</window>"));
  ASSERT_EQ(2, window_root_->ChildrenCount());

  Diadem::Entity* const multi = window_root_->ChildAt(1);
  ASSERT_EQ(3, multi->ChildrenCount());
  Diadem::Entity* const label1 = multi->ChildAt(1);
  Diadem::LabelGroup* const label_group =
      dynamic_cast<Diadem::LabelGroup*>(multi->ChildAt(2));
  Diadem::Entity* const group_label = label_group->GetLabel();
  Diadem::Entity* const label2 = window_root_->FindByName("l2");

  NSView* const l1_ref =
      reinterpret_cast<NSView*>(label1->GetNative()->GetNativeRef());
  NSView* const l2_ref =
      reinterpret_cast<NSView*>(label2->GetNative()->GetNativeRef());
  NSView* const gl_ref =
      reinterpret_cast<NSView*>(group_label->GetNative()->GetNativeRef());

  ASSERT_FALSE(group_label == NULL);
  ASSERT_FALSE(label2 == NULL);

  EXPECT_EQ(0, multi->GetProperty(Diadem::kPropValue).Coerce<int32_t>());
  EXPECT_FALSE([l1_ref isHidden]);
  EXPECT_TRUE([l2_ref isHidden]);
  EXPECT_TRUE([gl_ref isHidden]);

  EXPECT_TRUE(multi->SetProperty(Diadem::kPropValue, 1));
  EXPECT_TRUE([l1_ref isHidden]);
  EXPECT_FALSE([l2_ref isHidden]);
  EXPECT_FALSE([gl_ref isHidden]);
}
