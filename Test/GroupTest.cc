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
#include "Diadem/LabelGroup.h"

// Layout tests involving nested containers.
class GroupTest : public WindowTestBase {
 protected:
  void VerifyButtonGrid();
};

// A text label, with a row of two button beneath it.
TEST_F(GroupTest, testTextAndButtonRow) {
  ReadWindowData(
      "<window text='Group'>"
        "<label text='Some text here' name='l'/>"
        "<group>"
          "<button text='Cancel' name='b1'/>"
          "<button text='OK' name='b2'/>"
        "</group>"
      "</window>");
  EXPECT_STREQ("Group", windowRoot_->GetText());

  Diadem::Entity* const label   = windowRoot_->FindByName("l");
  Diadem::Entity* const button1 = windowRoot_->FindByName("b1");
  Diadem::Entity* const button2 = windowRoot_->FindByName("b2");
  ASSERT_FALSE(label == NULL);
  ASSERT_FALSE(button1 == NULL);
  ASSERT_FALSE(button2 == NULL);
  EXPECT_STREQ("Some text here", label->GetText());
  EXPECT_STREQ("Cancel", button1->GetText());
  EXPECT_STREQ("OK",     button2->GetText());

  Diadem::Layout* const labelL   = label->GetLayout();
  Diadem::Layout* const button1L = button1->GetLayout();
  Diadem::Layout* const button2L = button2->GetLayout();
  ASSERT_FALSE(labelL == NULL);
  ASSERT_FALSE(button1L == NULL);
  ASSERT_FALSE(button2L == NULL);

  const Diadem::Location locG = button1L->GetLayoutParent()->GetLocation();
  const Diadem::Location locL = labelL->GetLocation();
  const Diadem::Location loc1 = button1L->GetLocation() + locG;
  const Diadem::Location loc2 = button2L->GetLocation() + locG;
  const Diadem::Size sizeL = labelL->GetSize();
  const Diadem::Size size1 = button1L->GetSize();
  const Diadem::Size size2 = button2L->GetSize();
  const Diadem::Size sizeW = windowRoot_->GetLayout()->GetSize();
  const Diadem::Spacing margins = GetWindowMargins();
  const long pad =
      std::max(labelL->GetPadding().bottom, button1L->GetPadding().top);

  EXPECT_EQ(margins.left, locL.x);
  EXPECT_EQ(margins.left, locG.x);
  EXPECT_EQ(margins.left, loc1.x);
  EXPECT_EQ(locL.y + sizeL.height + pad, locG.y);
  EXPECT_EQ(sizeW.width - margins.right, loc2.x + size2.width);
  EXPECT_EQ(sizeW.height - margins.bottom, loc1.y + size1.height);
  EXPECT_EQ(sizeW.height - margins.bottom, loc2.y + size2.height);
  EXPECT_EQ(locL.y + sizeL.height + pad, loc1.y);
  EXPECT_EQ(loc1.y, loc2.y);
}

// A grid should look the same whether you do it rows first or columns first.
void GroupTest::VerifyButtonGrid() {
  Diadem::Layout* buttons[4];
  char name[2] = { '\0', '\0' };
  int i;

  for (i = 0; i < 4; ++i) {
    name[0] = '1'+i;
    buttons[i] = windowRoot_->FindByName(name)->GetLayout();
    ASSERT_FALSE(buttons[i] == NULL);
  }

  Diadem::Location locs[4];
  Diadem::Size sizes[4];
  const Diadem::Size sizeW =
      windowRoot_->GetProperty(Diadem::kPropSize).Coerce<Diadem::Size>();
  const Diadem::Spacing margins = GetWindowMargins();
  const Diadem::Spacing pad = buttons[0]->GetPadding();

  for (i = 0; i < 4; ++i) {
    locs[i] = buttons[i]->GetLocation() +
        buttons[i]->GetLayoutParent()->GetViewLocation();
    sizes[i] = buttons[i]->GetSize();
  }

  EXPECT_EQ(margins.left, locs[0].x);
  EXPECT_EQ(margins.top, locs[0].y);
  EXPECT_EQ(locs[0].x + sizes[0].width + pad.right, locs[1].x);
  EXPECT_EQ(margins.top, locs[1].y);
  EXPECT_EQ(sizeW.width - margins.right, locs[1].x+sizes[1].width);
  EXPECT_EQ(margins.left, locs[2].x);
  EXPECT_EQ(locs[0].y + sizes[0].height + pad.bottom, locs[2].y);
  EXPECT_EQ(sizeW.width  - margins.right, locs[3].x+sizes[3].width);
  EXPECT_EQ(sizeW.height - margins.bottom, locs[3].y+sizes[3].height);
}

// A 2x2 grid of buttons as a column of two rows.
TEST_F(GroupTest, testColumnOfRows)
{
  ReadWindowData(
      "<window>"
        "<group direction='column'>"
          "<group direction='row'>"
            "<button text='Button' name='1'/>"
            "<button text='Button' name='2'/>"
          "</group>"
          "<group direction='row'>"
            "<button text='Button' name='3'/>"
            "<button text='Button' name='4'/>"
          "</group>"
        "</group>"
      "</window>");

  VerifyButtonGrid();
}

// A 2x2 grid of buttons as a row of two columns.
TEST_F(GroupTest, testRowOfColumns) {
  ReadWindowData(
      "<window>"
        "<group direction='row'>"
          "<group direction='column'>"
            "<button text='Button' name='1'/>"
            "<button text='Button' name='3'/>"
          "</group>"
          "<group direction='column'>"
            "<button text='Button' name='2'/>"
            "<button text='Button' name='4'/>"
          "</group>"
        "</group>"
      "</window>");

  VerifyButtonGrid();
}

// Basic "row in a column" case.
TEST_F(GroupTest, testRowCross) {
  ReadWindowData(
      "<window>"
        "<check text='Check' name='check'/>"
        "<group>"
          "<button text='Button' name='button'/>"
        "</group>"
      "</window>");

  Diadem::Entity* const check  = windowRoot_->FindByName("check");
  Diadem::Entity* const button = windowRoot_->FindByName("button");
  ASSERT_FALSE(check == NULL);
  ASSERT_FALSE(button == NULL);

  Diadem::Layout* const checkL  = check->GetLayout();
  Diadem::Layout* const buttonL = button->GetLayout();
  ASSERT_FALSE(checkL == NULL);
  ASSERT_FALSE(buttonL == NULL);

  const Diadem::Location locC = checkL->GetLocation();
  const Diadem::Location locB =
      buttonL->GetLocation() + buttonL->GetLayoutParent()->GetLocation();
  const Diadem::Size sizeC = checkL->GetSize();
  const Diadem::Size sizeB = buttonL->GetSize();
  const Diadem::Size sizeW =
      windowRoot_->GetProperty(Diadem::kPropSize).Coerce<Diadem::Size>();
  const Diadem::Spacing margins = GetWindowMargins();

  EXPECT_EQ(
      locC.y+sizeC.height + std::max(buttonL->GetPadding().top,
      checkL->GetPadding().bottom), locB.y);
  EXPECT_EQ(sizeW.height - margins.bottom, locB.y+sizeB.height);
}

// Box is different from group because the native control is a superview, which
// requires different handling of coordinates.
TEST_F(GroupTest, testBox) {
  ReadWindowData(
      "<window text='testBox'>"
        "<box name='b'>"
          "<label text='Help!' name='h'/>"
        "</box>"
      "</window>");

  Diadem::Entity* const box = windowRoot_->FindByName("b");
  Diadem::Entity* const label = windowRoot_->FindByName("h");
  ASSERT_FALSE(box == NULL);
  ASSERT_FALSE(label == NULL);

  Diadem::Layout* const box_L = box->GetLayout();
  Diadem::Layout* const label_L = label->GetLayout();
  const Diadem::Size win_size = windowRoot_->GetLayout()->GetSize();
  const Diadem::Size box_size = box_L->GetSize();
  const Diadem::Size label_size = label_L->GetSize();
  const Diadem::Location box_loc = box_L->GetLocation();
  const Diadem::Location label_loc = label_L->GetLocation();
  const Diadem::Spacing margins = GetWindowMargins();
  const Diadem::Spacing box_margins =
      box->GetProperty(Diadem::kPropMargins).Coerce<Diadem::Spacing>();

  EXPECT_EQ(margins.left, box_loc.x);
  EXPECT_EQ(box_margins.left, label_loc.x);
  EXPECT_EQ(margins.top, box_loc.y);
  EXPECT_EQ(box_margins.top, label_loc.y);
  EXPECT_EQ(win_size.width, box_loc.x + box_size.width + margins.right);
  EXPECT_EQ(box_size.width, label_loc.x + label_size.width + box_margins.right);
  EXPECT_EQ(win_size.height, box_loc.y + box_size.height + margins.bottom);
  EXPECT_EQ(
      box_size.height,
      label_loc.y + label_size.height + box_margins.bottom);
}

// Simple case for column label group
TEST_F(GroupTest, testColumnLabelGroupSimple) {
  ReadWindowData(
      "<window text='testColumnLabelGroupSimple'>"
        "<labelgroup text='Exit:' width='fill'>"
          "<label text='Stage left'/>"
        "</labelgroup>"
      "</window>");
  ASSERT_EQ(1, windowRoot_->ChildrenCount());

  Diadem::LabelGroup *label_group =
      dynamic_cast<Diadem::LabelGroup*>(windowRoot_->ChildAt(0));
  ASSERT_FALSE(label_group == NULL);
  Diadem::ColumnLabelLayout *layout =
      dynamic_cast<Diadem::ColumnLabelLayout*>(label_group->GetLayout());
  Diadem::Entity *label = label_group->GetLabel();
  Diadem::Entity *content = label_group->GetContent();
  ASSERT_FALSE(layout == NULL);
  ASSERT_FALSE(label == NULL);
  ASSERT_FALSE(content == NULL);

  EXPECT_EQ(Diadem::kSizeFill, label_group->GetLayout()->GetHSizeOption());

  EXPECT_STREQ(Diadem::kTypeNameLabel, label->GetTypeName());
  EXPECT_STREQ(
      "Exit:",
      label->GetProperty(
          Diadem::kPropText).Coerce<Diadem::String>().Get());
  ASSERT_EQ(1, content->ChildrenCount());

  Diadem::Entity *content_label = content->ChildAt(0);

  EXPECT_STREQ(
      "Stage left",
      content_label->GetProperty(
          Diadem::kPropText).Coerce<Diadem::String>().Get());

  const Diadem::Spacing margins = GetWindowMargins();
  const Diadem::Layout *label_L = label->GetLayout();
  const Diadem::Layout *content_label_L = content_label->GetLayout();
  ASSERT_FALSE(label_L == NULL);
  ASSERT_FALSE(content_label_L == NULL);

  const Diadem::Location label_loc = label_L->GetViewLocation();
  const Diadem::Location content_label_loc = content_label_L->GetViewLocation();
  const Diadem::Size label_size = label_L->GetSize();
  const Diadem::Size content_label_size = content_label_L->GetSize();
  const Diadem::Spacing padding = label_L->GetPadding();

  EXPECT_EQ(margins.left, label_loc.x);
  EXPECT_EQ(margins.top, label_loc.y);
  EXPECT_EQ(margins.top, content_label_loc.y);
  EXPECT_EQ(
      label_loc.x + label_size.width + padding.right,
      content_label_loc.x);
}

// Tests two adjacent labelgroups
TEST_F(GroupTest, testColumnLabelGroupDouble) {
  ReadWindowData(
      "<window text='testColumnLabelGroupDouble'>"
        "<labelgroup text='AA' type='column'>"
          "<label text='One'/>"
        "</labelgroup>"
        "<labelgroup text='AAAA' type='column'>"
          "<label text='Two'/>"
        "</labelgroup>"
      "</window>");
  ASSERT_EQ(2, windowRoot_->ChildrenCount());

  Diadem::LabelGroup *lg1 =
      dynamic_cast<Diadem::LabelGroup*>(windowRoot_->ChildAt(0));
  Diadem::LabelGroup *lg2 =
      dynamic_cast<Diadem::LabelGroup*>(windowRoot_->ChildAt(1));
  ASSERT_FALSE(lg1 == NULL);
  ASSERT_FALSE(lg2 == NULL);

  Diadem::Entity *label1 = lg1->GetLabel();
  Diadem::Entity *label2 = lg2->GetLabel();
  ASSERT_FALSE(label1 == NULL);
  ASSERT_FALSE(label2 == NULL);

  EXPECT_STREQ("AA",
      label1->GetProperty(
          Diadem::kPropText).Coerce<Diadem::String>().Get());
  EXPECT_STREQ("AAAA",
      label2->GetProperty(
          Diadem::kPropText).Coerce<Diadem::String>().Get());

  EXPECT_STREQ(
      label1->GetLayout()->GetWidthName().Get(),
      label2->GetLayout()->GetWidthName().Get());
  EXPECT_LT(0, strlen(label1->GetLayout()->GetWidthName().Get()));

  const Diadem::Size l1_size =
      label1->GetProperty(Diadem::kPropSize).Coerce<Diadem::Size>();
  const Diadem::Size l2_size =
      label1->GetProperty(Diadem::kPropSize).Coerce<Diadem::Size>();

  EXPECT_EQ(l1_size.width, l2_size.width);
}

// A group within a group.
TEST_F(GroupTest, testNestedGroups) {
  ReadWindowData(
      "<window text='testNestedGroups'>"
        "<group name='g1'>"
          "<group name='g2'>"
            "<button name='b1' text='Top'/>"
          "</group>"
        "</group>"
        "<group name='g3'>"
          "<button name='b2' text='Bottom'/>"
        "</group>"
      "</window>");

  Diadem::Entity* const g1 = windowRoot_->ChildAt(0);
  ASSERT_FALSE(g1 == NULL);
  Diadem::Entity* const g2 = g1->ChildAt(0);
  ASSERT_FALSE(g2 == NULL);
  Diadem::Entity* const g3 = windowRoot_->ChildAt(1);
  ASSERT_FALSE(g3 == NULL);

  Diadem::Entity* const b1 = g2->ChildAt(0);
  ASSERT_FALSE(b1 == NULL);
  Diadem::Entity* const b2 = g3->ChildAt(0);
  ASSERT_FALSE(b2 == NULL);

  const Diadem::Spacing pad_b1 = b1->GetLayout()->GetPadding();
  const Diadem::Spacing pad_g1 = g1->GetLayout()->GetPadding();
  const Diadem::Spacing pad_g2 = g2->GetLayout()->GetPadding();
  const Diadem::Spacing pad_g3 = g3->GetLayout()->GetPadding();
  const Diadem::Size size_b1 = b1->GetLayout()->GetSize();
  const Diadem::Size size_g1 = g1->GetLayout()->GetSize();
  const Diadem::Location loc_g1 = g1->GetLayout()->GetLocation();
  const Diadem::Location loc_g2 = g2->GetLayout()->GetLocation();
  const Diadem::Location loc_g3 = g3->GetLayout()->GetLocation();
  const Diadem::Location loc_b1 = b1->GetLayout()->GetLocation();
  const Diadem::Location loc_b2 = b2->GetLayout()->GetLocation();

  // The first item in a container should be at 0, 0
  EXPECT_EQ(0, loc_g2.x);
  EXPECT_EQ(0, loc_g2.y);
  EXPECT_EQ(0, loc_b1.x);
  EXPECT_EQ(0, loc_b1.y);
  EXPECT_EQ(0, loc_b2.x);
  EXPECT_EQ(0, loc_b2.y);

  // The button and its enclosing groups should all have the same padding.
  EXPECT_EQ(pad_b1.bottom, pad_g2.bottom);
  EXPECT_EQ(pad_b1.bottom, pad_g1.bottom);

  EXPECT_EQ(size_b1.height, size_g1.height);
  EXPECT_EQ(
      loc_g3.y,
      loc_g1.y + size_g1.height + std::max(pad_g1.bottom, pad_g3.top));
}

// A group's baseline is the same as its first child
TEST_F(GroupTest, testGroupBaseline) {
  ReadWindowData(
      "<window text='testGroupBaseline'>"
        "<label text='Sky'/>"
        "<group>"
          "<label text='Blue'/>"
        "</group>"
      "</window>");

  const Diadem::Entity* const l1 = windowRoot_->ChildAt(0);
  const Diadem::Entity* const g = windowRoot_->ChildAt(1);
  const Diadem::Entity* const l2 = g->ChildAt(0);

  const int32_t b1 = l1->GetLayout()->GetBaseline();
  const int32_t b2 = l2->GetLayout()->GetBaseline();
  const int32_t bg = g->GetLayout()->GetBaseline();

  EXPECT_EQ(b1, bg);
  EXPECT_EQ(b2, bg);
}

TEST_F(GroupTest, testNestedGroupFill) {
  ReadWindowData(
      "<window width='20em'>"
        "<group width='fill'><group width='fill'>"
          "<label width='fill'/>"
        "</group></group>"
        "<group width='fill'>"
          "<label width='fill'/>"
        "</group>"
        "<label width='fill'/>"
      "</window>");

  const Diadem::Entity* const group1 = windowRoot_->ChildAt(0);
  const Diadem::Entity* const group11 = group1->ChildAt(0);
  const Diadem::Entity* const label1 = group1->ChildAt(0);
  const Diadem::Entity* const group2 = windowRoot_->ChildAt(1);
  const Diadem::Entity* const label2 = group2->ChildAt(0);
  const Diadem::Entity* const label3 = windowRoot_->ChildAt(2);

  const int32_t width = label3->GetLayout()->GetSize().width;

  EXPECT_EQ(width, group1->GetLayout()->GetSize().width);
  EXPECT_EQ(width, group11->GetLayout()->GetSize().width);
  EXPECT_EQ(width, group2->GetLayout()->GetSize().width);
  EXPECT_EQ(width, label1->GetLayout()->GetSize().width);
  EXPECT_EQ(width, label2->GetLayout()->GetSize().width);
}
