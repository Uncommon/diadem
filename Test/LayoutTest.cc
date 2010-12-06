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
#include "Diadem/Layout.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

class LayoutTest : public WindowTestBase {
};

TEST_F(LayoutTest, testRowFill) {
  ReadWindowData(
    "<window text='testRowFill' width='30em' direction='row'>"
      "<label width='fill' text='Wrapping text' name='1'/>"
      "<label text='Five' name='2'/>"
    "</window>");

  Diadem::Entity* const label1 = windowRoot_->FindByName("1");
  Diadem::Entity* const label2 = windowRoot_->FindByName("2");
  ASSERT_FALSE(label1 == NULL);
  ASSERT_FALSE(label2 == NULL);

  Diadem::Layout* const label1L = label1->GetLayout();
  Diadem::Layout* const label2L = label2->GetLayout();
  ASSERT_FALSE(label1L == NULL);
  ASSERT_FALSE(label2L == NULL);

  const Diadem::Location loc1 = label1L->GetLocation();
  const Diadem::Location loc2 = label2L->GetLocation();
  const Diadem::Size size1 = label1L->GetSize();
  const Diadem::Size size2 = label2L->GetSize();
  const Diadem::Spacing pad1 = label1L->GetPadding();
  const Diadem::Spacing pad2 = label1L->GetPadding();
  const Diadem::Spacing margins = GetWindowMargins();
  const Diadem::Size wSize = windowRoot_->GetLayout()->GetSize();

  EXPECT_EQ(size2.height, size1.height);
  EXPECT_EQ(loc1.x, margins.left);
  EXPECT_EQ(loc2.x + size2.width, wSize.width - margins.right);
  EXPECT_EQ(
      loc1.x + size1.width + std::max(pad1.right, pad2.left),
      loc2.x);
}

TEST_F(LayoutTest, testEditFill) {
  ReadWindowData(
    "<window text='testEditFill'>"
      "<edit width='5em'/>"
      "<edit width='fill'/>"
    "</window>");
  ASSERT_EQ(windowRoot_->ChildrenCount(), 2);

  Diadem::Entity* const label1 = windowRoot_->ChildAt(0);
  Diadem::Entity* const label2 = windowRoot_->ChildAt(1);
  ASSERT_FALSE(label1 == NULL);
  ASSERT_FALSE(label2 == NULL);

  Diadem::Layout* const label1L = label1->GetLayout();
  Diadem::Layout* const label2L = label2->GetLayout();
  ASSERT_FALSE(label1L == NULL);
  ASSERT_FALSE(label2L == NULL);

  EXPECT_EQ(label1L->GetLocation().x, label2L->GetLocation().x);
  EXPECT_EQ(label1L->GetSize().width, label2L->GetSize().width);
}

TEST_F(LayoutTest, testInLayoutFirst) {
  ReadWindowData(
    "<window direction='row'>"
      "<button text='C' name='c'/>"
      "<button text='B' name='b'/>"
      "<button text='A' name='a'/>"
    "</window>");

  Diadem::Entity* const buttonA = windowRoot_->FindByName("a");
  Diadem::Entity* const buttonB = windowRoot_->FindByName("b");
  Diadem::Entity* const buttonC = windowRoot_->FindByName("c");
  ASSERT_FALSE(buttonA == NULL);
  ASSERT_FALSE(buttonB == NULL);
  ASSERT_FALSE(buttonC == NULL);

  Diadem::Layout* const buttonAL = buttonA->GetLayout();
  Diadem::Layout* const buttonBL = buttonB->GetLayout();
  Diadem::Layout* const buttonCL = buttonC->GetLayout();

  const Diadem::Spacing margins = GetWindowMargins();
  Diadem::Size wSize = windowRoot_->GetLayout()->GetSize();
  Diadem::Size bSize = buttonAL->GetSize();
  Diadem::Location bLoc = buttonAL->GetLocation();

  EXPECT_EQ(wSize.width - margins.right, bLoc.x + bSize.width) << "Before hide";

  EXPECT_TRUE(buttonCL->IsVisible());
  buttonCL->SetInLayout(false);
  EXPECT_FALSE(buttonCL->IsVisible());
  windowRoot_->GetLayout()->ResizeToMinimum();

  wSize = windowRoot_->GetLayout()->GetSize();
  bSize = buttonAL->GetSize();
  bLoc = buttonAL->GetLocation();

  EXPECT_EQ(wSize.width - margins.right, bLoc.x + bSize.width) << "After hide";

  bLoc = buttonBL->GetLocation();

  EXPECT_EQ(margins.left, bLoc.x);
}

TEST_F(LayoutTest, testInLayoutMiddle) {
  ReadWindowData(
    "<window direction='row'>"
      "<button text='A' name='a'/>"
      "<button text='B' name='b'/>"
      "<button text='C' name='c'/>"
    "</window>");

  Diadem::Entity* const buttonA = windowRoot_->FindByName("a");
  Diadem::Entity* const buttonB = windowRoot_->FindByName("b");
  Diadem::Entity* const buttonC = windowRoot_->FindByName("c");
  ASSERT_FALSE(buttonA == NULL);
  ASSERT_FALSE(buttonB == NULL);
  ASSERT_FALSE(buttonC == NULL);

  Diadem::Layout* const buttonAL = buttonA->GetLayout();
  Diadem::Layout* const buttonBL = buttonB->GetLayout();
  Diadem::Layout* const buttonCL = buttonC->GetLayout();

  EXPECT_TRUE(buttonAL->IsVisible());
  EXPECT_TRUE(buttonAL->IsInLayout());
  EXPECT_TRUE(buttonBL->IsVisible());
  EXPECT_TRUE(buttonBL->IsInLayout());
  EXPECT_TRUE(buttonCL->IsVisible());
  EXPECT_TRUE(buttonCL->IsInLayout());
  EXPECT_LT(buttonBL->GetLocation().x, buttonCL->GetLocation().x);

  buttonBL->SetInLayout(false);
  EXPECT_TRUE(!buttonBL->IsVisible());
  EXPECT_TRUE(!buttonBL->IsInLayout());

  windowRoot_->GetLayout()->ResizeToMinimum();
  EXPECT_EQ(
      buttonAL->GetLocation().x + buttonAL->GetSize().width +
        std::max(buttonAL->GetPadding().right, buttonCL->GetPadding().left),
      buttonCL->GetLocation().x);
}

TEST_F(LayoutTest, testExplicitSize) {
  ReadWindowData(
    "<window title='Explicit'>"
      "<label width='5em' text='a'/>"
    "</window>");

  Diadem::Entity* const label = windowRoot_->ChildAt(0);
  ASSERT_FALSE(label == NULL);

  Diadem::Layout* const labelL = label->GetLayout();
  Diadem::Native* const native = label->GetNative();
  ASSERT_FALSE(labelL == NULL);
  ASSERT_FALSE(native == NULL);

  const Diadem::PlatformMetrics &metrics = native->GetPlatformMetrics();
  const Diadem::Size size = labelL->GetSize();

  EXPECT_EQ(Diadem::kSizeExplicit, labelL->GetHSizeOption());
  EXPECT_EQ(metrics.emSize * 5, size.width);
}

TEST_F(LayoutTest, testAlignment) {
  ReadWindowData(
    "<window text='Test' direction='column'>"
      "<label text='Some text that&apos;s wider than the buttons'/>"
      "<button text='Start'  name='a' align='start'/>"
      "<button text='Center' name='b' align='center'/>"
      "<button text='End'    name='c' align='end'/>"
    "</window>");
  ASSERT_EQ(4, windowRoot_->ChildrenCount());
  ASSERT_EQ(
      Diadem::LayoutContainer::kLayoutColumn,
      windowRoot_->GetProperty(Diadem::kPropDirection).Coerce<int>());

  Diadem::Entity *label = windowRoot_->ChildAt(0);
  const Diadem::Size wSize = windowRoot_->GetLayout()->GetSize();
  const Diadem::Spacing margins = GetWindowMargins();
  Diadem::Entity *entity;

  entity = windowRoot_->FindByName("a");
  ASSERT_FALSE(entity == NULL);
  ASSERT_GT(
      label->GetLayout()->GetSize().width,
      entity->GetLayout()->GetSize().width);
  EXPECT_EQ(margins.left, entity->GetLayout()->GetLocation().x);

  entity = windowRoot_->FindByName("b");
  ASSERT_FALSE(entity == NULL);

  const int leftSpace  = entity->GetLayout()->GetLocation().x;
  const int rightSpace =
      wSize.width - (leftSpace + entity->GetLayout()->GetSize().width);

  EXPECT_GE(1, abs(leftSpace - rightSpace));
  EXPECT_GT(leftSpace, margins.left);

  entity = windowRoot_->FindByName("c");
  ASSERT_FALSE(entity == NULL);
  EXPECT_EQ(
      wSize.width - margins.right,
      entity->GetLayout()->GetLocation().x +
      entity->GetLayout()->GetSize().width);
}

TEST_F(LayoutTest, testOneButton) {
  ReadWindowData(
    "<window text='Test'><button text='OK'/></window>");
  EXPECT_STREQ("Test", windowRoot_->GetText());
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Diadem::Entity* const button = windowRoot_->ChildAt(0);
  EXPECT_STREQ("OK", button->GetText());

  const Diadem::Location loc = button->GetLayout()->GetLocation();
  const Diadem::Size bSize = button->GetLayout()->GetSize();
  const Diadem::Size wSize = windowRoot_->GetLayout()->GetSize();
  const Diadem::Spacing margins = GetWindowMargins();

  EXPECT_EQ(margins.left, loc.x) << "l margin";
  EXPECT_EQ(margins.top, loc.y)  << "t margin";
  EXPECT_EQ(wSize.width - margins.right,   loc.x + bSize.width)  << "r margin";
  EXPECT_EQ(wSize.height - margins.bottom, loc.y + bSize.height) << "b margin";
}

TEST_F(LayoutTest, testTwoButtonRow) {
  ReadWindowData(
    "<window text='Test' direction='row'>"
      "<button text='Cancel' type='cancel'/>"
      "<button text='OK' type='default'/>"
    "</window>");

  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity* const button1 = windowRoot_->ChildAt(0);
  Diadem::Entity* const button2 = windowRoot_->ChildAt(1);
  EXPECT_STREQ("Cancel", button1->GetText());
  EXPECT_STREQ("OK", button2->GetText());

  const Diadem::Location loc1  = button1->GetLayout()->GetLocation();
  const Diadem::Location loc2  = button2->GetLayout()->GetLocation();
  const Diadem::Size     size1 = button1->GetLayout()->GetSize();
  const Diadem::Size     size2 = button2->GetLayout()->GetSize();
  const Diadem::Size     wSize = windowRoot_->GetLayout()->GetSize();
  const Diadem::Spacing  margins = GetWindowMargins();
  const Diadem::Spacing  padding = button1->GetLayout()->GetPadding();

  EXPECT_EQ(margins.left, loc1.x) << "Left margin";
  EXPECT_EQ(margins.top, loc1.y)  << "Top margin 1";
  EXPECT_EQ(margins.top, loc2.y)  << "Top margin 2";
  EXPECT_EQ(margins.left + size1.width + padding.right, loc2.x) << "Padding";
  EXPECT_EQ(
      wSize.height - margins.bottom,
      loc1.y + size1.height) << "b margin 1";
  EXPECT_EQ(
      wSize.height - margins.bottom,
      loc2.y + size2.height) << "b margin 2";
  EXPECT_EQ(wSize.width - margins.right, loc2.x + size2.width) << "r margin";
}

TEST_F(LayoutTest, testTwoButtonColumn) {
  ReadWindowData(
    "<window title='Test' direction='column'>"
      "<button text='Button'/>"
      "<button text='Button'/>"
    "</window>");

  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity* const button1 = windowRoot_->ChildAt(0);
  Diadem::Entity* const button2 = windowRoot_->ChildAt(1);
  EXPECT_STREQ("Button", button1->GetText());
  EXPECT_STREQ("Button", button2->GetText());

  const Diadem::Location loc1  = button1->GetLayout()->GetLocation();
  const Diadem::Location loc2  = button2->GetLayout()->GetLocation();
  const Diadem::Size     size1 = button1->GetLayout()->GetSize();
  const Diadem::Size     size2 = button2->GetLayout()->GetSize();
  const Diadem::Size     wSize = windowRoot_->GetLayout()->GetSize();
  const Diadem::Spacing  margins = GetWindowMargins();
  const Diadem::Spacing  padding = button1->GetLayout()->GetPadding();

  EXPECT_EQ(margins.top, loc1.y)  << "Top margin";
  EXPECT_EQ(margins.left, loc1.x) << "Left margin 1";
  EXPECT_EQ(margins.left, loc2.x) << "Left margin 2";
  EXPECT_EQ(margins.top + size1.height + padding.bottom, loc2.y)  << "Padding";
  EXPECT_EQ(wSize.width - margins.right, loc1.x + size1.width) << "r margin 1";
  EXPECT_EQ(wSize.width - margins.right, loc2.x + size2.width) << "r margin 2";
  EXPECT_EQ(wSize.height - margins.bottom, loc2.y + size2.height) << "r margin";
}

TEST_F(LayoutTest, testLabelButtonRow) {
  ReadWindowData(
    "<window text='Test' direction='row'>"
      "<label text='Some text:'/>"
      "<button text='OK'/>"
    "</window>");

  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity* const label  = windowRoot_->ChildAt(0);
  Diadem::Entity* const button = windowRoot_->ChildAt(1);
  EXPECT_STREQ("Some text:", label->GetText());
  EXPECT_STREQ("OK", button->GetText());

  const Diadem::Location locL = label->GetLayout()->GetLocation();
  const Diadem::Location locB = button->GetLayout()->GetLocation();
  const Diadem::Size sizeL = label->GetLayout()->GetSize();
  const Diadem::Size sizeB = button->GetLayout()->GetSize();
  const Diadem::Spacing margins = GetWindowMargins();
  const int offset =
      button->GetLayout()->GetBaseline() - label->GetLayout()->GetBaseline();

  EXPECT_GE(offset, 0) << "Button should have lower baseline";
  EXPECT_EQ(margins.left, locL.x);
  EXPECT_EQ(margins.top, locB.y);
  EXPECT_EQ(margins.top + offset, locL.y) << "Baseline alignment";
}

TEST_F(LayoutTest, testSizeParse) {
  ReadWindowData(
    "<window title='Test'>"
      "<label text='Default' width='default'/>"
      "<label text='Fit'     width='fit'/>"
      "<label text='Fill'    width='fill'/>"
    "</window>");

#define HSizeOption(_e_) \
    _e_->GetProperty(Diadem::kPropWidthOption).Coerce<int32_t>()

  ASSERT_EQ(3, windowRoot_->ChildrenCount());
  EXPECT_EQ(Diadem::kSizeDefault, HSizeOption(windowRoot_->ChildAt(0)));
  EXPECT_EQ(Diadem::kSizeFit,     HSizeOption(windowRoot_->ChildAt(1)));
  EXPECT_EQ(Diadem::kSizeFill,    HSizeOption(windowRoot_->ChildAt(2)));
}

TEST_F(LayoutTest, testHSeparator) {
  ReadWindowData(
    "<window text='testHSeparator' direction='row'>"
      "<label text='Separator'/>"
      "<separator/>"
    "</window>");
  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity* const label = windowRoot_->ChildAt(0);
  Diadem::Entity* const sep = windowRoot_->ChildAt(1);
  ASSERT_FALSE(label == NULL);
  ASSERT_FALSE(sep   == NULL);

  const Diadem::Size sizeL = label->GetLayout()->GetSize();
  const Diadem::Size sizeS = sep->GetLayout()->GetSize();

  EXPECT_TRUE(sizeS.width > 0);
  EXPECT_EQ(sizeL.height, sizeS.height);
}

TEST_F(LayoutTest, testVSeparator) {
  ReadWindowData(
    "<window text='S'>"
      "<label text='Separator'/>"
      "<separator/>"
    "</window>");
  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::Entity* const label = windowRoot_->ChildAt(0);
  Diadem::Entity* const sep = windowRoot_->ChildAt(1);
  ASSERT_FALSE(label == NULL);
  ASSERT_FALSE(sep   == NULL);

  const Diadem::Size sizeL = label->GetLayout()->GetSize();
  const Diadem::Size sizeS = sep->GetLayout()->GetSize();

  EXPECT_GT(sizeS.height, 0);
  EXPECT_EQ(sizeL.width, sizeS.width);
}
