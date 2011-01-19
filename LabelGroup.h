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

#ifndef DIADEM_LABELGROUP_H_
#define DIADEM_LABELGROUP_H_

#include "Diadem/Layout.h"

namespace Diadem {

extern const PropertyName kPropLabelGroupType, kPropColumnWidthName;
extern const TypeName kTypeNameLabelGroup;
extern const StringConstant kLabelGroupTypeColumn, kLabelGroupTypeIndent;

// A label grop is a text label and an associated group of controls. Various
// layouts are possible, and the one to use could depend on the platform.
class LabelGroup : public Entity {
 public:
  LabelGroup() : label_(NULL), content_(NULL) {}

  // Creates the label and content child entities.
  virtual void InitializeProperties(
      const PropertyMap &properties,
      const Factory &factory);

  // Reads the "type" property to specify layout: column, indent, etc.
  virtual Bool SetProperty(PropertyName name, const Value &value);

  // Children are added to the content group instead.
  virtual void AddChild(Entity *child);

  // Uses a default layout if none has been specified.
  virtual void Finalize();

  Entity* GetLabel()             { return label_; }
  const Entity* GetLabel() const { return label_; }
  Entity* GetContent()             { return content_; }
  const Entity* GetContent() const { return content_; }

 protected:
  Entity *label_;    // The text label.
  Entity *content_;  // The content group.

  void InitializeLayout(Layout *layout);
};

// Abstract superclass for the implementations of various label group styles.
class LabelGroupLayout : public Group {
 public:
  explicit LabelGroupLayout(LabelGroup *group) : label_group_(group) {}

 protected:
  LabelGroup *label_group_;
};

// A two-column layout: labels are right-aligned in the left column, and the
// content is the right column. Multiple adjacent label groups have the same
// width for the label column, implemented using named widths.
class ColumnLabelLayout : public LabelGroupLayout {
 public:
  explicit ColumnLabelLayout(LabelGroup *group) : LabelGroupLayout(group) {}

  virtual void Finalize();
  virtual Bool SetProperty(PropertyName name, const Value &value);
};

// The label appears left-aligned above, and the content is indented below.
class IndentLabelLayout : public LabelGroupLayout {
 public:
  explicit IndentLabelLayout(LabelGroup *group) : LabelGroupLayout(group) {}
  // TODO(catmull): finish implementation
};

}  // namespace Diadem

#endif  // DIADEM_LABELGROUP_H_
