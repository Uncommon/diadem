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

#ifndef DIADEM_ENTITY_H_
#define DIADEM_ENTITY_H_

#include "Diadem/Wrappers.h"

namespace Diadem {

class Factory;
class Layout;
class Native;
class Value;
class Window;

// Having this makes it easier to declare lots of const char* const variables.
typedef const char* StringConstant;
typedef StringConstant PropertyName;
typedef StringConstant TypeName;
typedef Map<String, Value> PropertyMap;

extern const PropertyName kPropName, kPropText, kPropEnabled;

// Basic object type: has a unique name, created from a resource.
class Entity : public Base {
 public:
  Entity();
  virtual ~Entity();  // all children are deleted

  // Initializes with the properties given in the resource file.
  virtual void InitializeProperties(
      const PropertyMap &properties,
      const Factory &factory);
  // Notification that the Factory has finished constructing the hierarchy.
  // Subclasses may override Finalize().
  void FactoryFinalize();

  // Entities form a tree with parent-child relationships.
  Entity* GetParent() { return parent_; }
  const Entity* GetParent() const { return parent_; }
  uint32_t ChildrenCount() const { return children_.size(); }
  Entity* ChildAt(uint32_t index) { return children_[index]; }
  const Entity* ChildAt(uint32_t index) const { return children_[index]; }
  virtual void AddChild(Entity *child);
  void RemoveChild(Entity *child);

  // Finds and entity by name, starting from this point in the hierarchy.
  Entity* FindByName(const char *name);

  // The purpose of a path is to have a unique string for an entity even when
  // the name is not specified. If it does have a name, the path is the name
  // in quotes. If it is the root entity, the path is /t where t is the type
  // (eg, /window). Otherwise, it's p/ti, where p is the parent path and i is
  // the 1-based index of the entity within its parent, by type.
  // Example: the second label in a window would be /window/label2
  String GetPath() const;
  // Returns the index of the child, counting only children with the same type
  // name.
  uint32_t ChildIndexByType(const Entity *child) const;

  // Returns the type name that would be used for the entity in a resource file.
  // Delegates to the Native or Layout object.
  String GetTypeName() const;

  /// Gets/sets the Layout helper object which handles all dialog layout.
  void SetLayout(Layout *layout);
  Layout* GetLayout()             { return layout_; }
  const Layout* GetLayout() const { return layout_; }

  // Gets/sets the Native helper object, containing the platform implementation.
  void SetNative(Native *native);
  Native* GetNative()             { return native_; }
  const Native* GetNative() const { return native_; }
  virtual void AddNative(Native *n);

  // SetWindow should only be called on the root entity.
  void SetWindow(Window *window) {
    DASSERT(parent_ == NULL);
    window_ = window;
  }
  Window* GetWindow();
  const Window* GetWindow() const;

  // Gets/sets a named property, which may be delegated to a helper.
  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

  // Shortcuts for when the caller knows Layout will handle the property.
  virtual bool SetLayoutProperty(PropertyName name, const Value &value);
  virtual Value GetLayoutProperty(PropertyName name) const;

  // Shortcuts for when the caller knows Native will handle the property.
  virtual bool SetNativeProperty(PropertyName name, const Value &value);
  virtual Value GetNativeProperty(PropertyName name) const;

  // Every Entity can have a name, which should be unique within the hierarchy
  // if it is not empty.
  void SetName(const char *name)
    { name_ = name; }
  const char* GetName() const
    { return name_; }

  // Shortcuts to setting/getting kPropText
  void SetText(const char *text);
  String GetText() const;

  // Notification that the control for the given entity was clicked. The
  // message is passed up the hierarchy until an entity has a button callback.
  virtual void Clicked(Entity *target);
  void Clicked() { Clicked(this); }

  typedef void (*ButtonCallback)(Entity *target, void *data);

  // Sets the function to be called when a control is clicked.
  void SetButtonCallback(ButtonCallback callback, void *data = NULL) {
    button_callback_ = callback;
    button_data_ = data;
  }

 protected:
  String name_;
  Entity *parent_;
  Array<Entity*> children_;
  Layout *layout_;
  Native *native_;
  Window *window_;
  ButtonCallback button_callback_;
  void *button_data_;

  // Called by FactoryFinalize() once the Native and Layout helpers have also
  // been finalized.
  virtual void Finalize() {}

  void SetParent(Entity *parent) { parent_ = parent; }

  // Called by AddChild and RemoveChild in case subclasses need to take
  // special action
  virtual void ChildAdded(Entity *child) {}
  virtual void ChildRemoved(Entity *child) {}

  // Add a new child recursively in case it has children
  void AddNativeChild(Entity *child);

 private:
  // Disallow copy and assign
  Entity(const Entity&);
  void operator=(const Entity*);
};

// Superclass for Layout and Native. In a previous incarnation, these features
// were all in one class, but some objects are not in the layout (like
// menu items), and some have no native controls (like groups).
class EntityDelegate : public Base {
 public:
  EntityDelegate() : entity_(NULL) {}
  virtual ~EntityDelegate() {}

  virtual void Finalize() {}

  void SetEntity(Entity *entity) { entity_ = entity; }
  Entity* GetEntity()             { return entity_; }
  const Entity* GetEntity() const { return entity_; }

  virtual String GetTypeName() const { return String(); }

  virtual void InitializeProperties(const PropertyMap &properties) {}

  virtual bool SetProperty(PropertyName name, const Value &value);
  virtual Value GetProperty(PropertyName name) const;

 protected:
  Entity *entity_;

 private:
  // Disallow copy and assign.
  EntityDelegate(const EntityDelegate&);
  void operator=(const EntityDelegate*);
};

// A connection between two entity values.
class Binding : public Entity {
 public:
  Binding() {}

  static const PropertyName
      kPropSource, kPropTarget, kPropFormat, kPropList, kPropTransform;
};

}  // namespace Diadem

#endif  // DIADEM_ENTITY_H_
