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

#include "Diadem/LabelGroup.h"
#include "Diadem/Factory.h"
#include "Diadem/Native.h"

namespace Diadem {

const PropertyName
    kPropLabelGroupType  = "type",
    kPropColumnWidthName = "colWidthName";

const TypeName kTypeNameLabelGroup = "labelgroup";

void LabelGroup::InitializeProperties(
      const PropertyMap &properties,
      const Factory &factory) {
  PropertyMap labelProperties;

  labelProperties.Insert(kPropTextAlign, String(kTextAlignRight));
  label_ = factory.CreateEntity(kTypeNameLabel, labelProperties);

  content_ = new Entity();
  content_->SetLayout(new Group());
  content_->GetLayout()->SetProperty(kPropDirection, Layout::kLayoutColumn);

  Entity::InitializeProperties(properties, factory);
}

Bool LabelGroup::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropText) == 0) {
    label_->SetProperty(kPropText, value);
    return true;
  }
  if (strcmp(name, kPropLabelGroupType) == 0) {
    DASSERT(layout_ == NULL);
    const String type = value.Coerce<String>();
    
    if (type == "column")
      InitializeLayout(new ColumnLabelLayout(this));
    else if (type == "indent")
      InitializeLayout(new IndentLabelLayout(this));
    return true;
  }
  return Entity::SetProperty(name, value);
}

void LabelGroup::AddChild(Entity *child) {
  if (layout_ == NULL)
    InitializeLayout(new ColumnLabelLayout(this));
  DASSERT(content_ != NULL);
  content_->AddChild(child);
}

void LabelGroup::Finalize() {
  // Bypass the override, which would add it to the content group
  Entity::AddChild(label_);
  Entity::AddChild(content_);
  label_->GetLayout()->SetWidthName(GetParent()->GetPath());

  if (GetLayout() == NULL)
    InitializeLayout(new ColumnLabelLayout(this));
}

void LabelGroup::InitializeLayout(Layout *layout) {
  SetLayout(layout);
  layout->ChildAdded(label_);
  layout->ChildAdded(content_);
}

Bool ColumnLabelLayout::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropColumnWidthName)) {
    label_group_->GetLabel()->SetProperty(kPropWidthName, value);
    return true;
  }
  return LabelGroupLayout::SetProperty(name, value);
}

void ColumnLabelLayout::Finalize() {
  direction_ = kLayoutRow;
  label_group_->GetContent()->SetProperty(kPropAlign, kAlignStart);

  Entity* const label = label_group_->GetLabel();
  DASSERT(label != NULL);
  Layout* const label_layout = label->GetLayout();
  DASSERT(label_layout != NULL);

  if (label_layout->GetWidthName().IsEmpty()) {
    DASSERT(label->GetParent() != NULL);
    label_layout->SetWidthName(label->GetParent()->GetPath());
  }
}

}  // namespace Diadem
