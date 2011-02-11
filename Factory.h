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

#ifndef DIADEM_FACTORY_H_
#define DIADEM_FACTORY_H_

#include "Diadem/Entity.h"

namespace Diadem {

// The Factory contains a registry of entity names and the types of objects
// that should be created for them.
class Factory : public Base {
 public:
  Factory() { RegisterBasicClasses(); }

  typedef Entity* (*CreateEntityFunction)();
  typedef Layout* (*CreateLayoutFunction)();
  typedef Native* (*CreateNativeFunction)();

  // For every class name there is a set of creation functions. The layout and
  // native creators may be NULL.
  struct CreatorFunctions {
    CreateEntityFunction entity_creator;
    CreateLayoutFunction layout_creator;
    CreateNativeFunction native_creator;
  };

  typedef Map<String, CreatorFunctions> CreationRegistry;
  typedef CreationRegistry::Pair CreationEntry;

  template <class T, class C>
  class Creator {
   public:
    static C* Create() { return new T(); }
  };

  // NoLayout is a placeholder class to simplify registering types that will
  // have no layout object.
  class NoLayout {};

  template <class C>
  class Creator<NoLayout, C> {
   public:
    static C* Create() { return NULL; }
  };

  const CreationRegistry& Registry() const
    { return registry_; }
  void RegisterCreator(
      const char *class_name,
      CreateEntityFunction entity_creator,
      CreateLayoutFunction layout_creator,
      CreateNativeFunction native_creator) {
    CreatorFunctions functions = {
        entity_creator, layout_creator, native_creator };
    registry_.Insert(String(class_name), functions);
  }

  // Registers a class name with no layout or native helper.
  template <class T>
  void Register(const char *class_name)
    { RegisterCreator(class_name, &Creator<T, Entity>::Create, NULL, NULL); }

  // Native subclasses should have typedefs named EntityType and LayoutType.
  template <class T>
  void RegisterNative(const char *class_name) {
    RegisterCreator(
        class_name,
        &Creator<typename T::EntityType, Entity>::Create,
        &Creator<typename T::LayoutType, Layout>::Create,
        &Creator<T, Native>::Create);
  }

  // Looks up the given class name and creates a new Entity with the
  // corresponding creator functions, initialized with the given properties.
  // Returns NULL if the name is not found.
  Entity* CreateEntity(
      const char *class_name, const PropertyMap &properties) const;

  bool IsRegistered(const char *class_name) {
    return registry_.Exists(class_name);
  }

  // Registers all standard, platform-independent classes.
  void RegisterBasicClasses();

 protected:
  CreationRegistry registry_;
};

// The FactorySession is used by the Parser object to construct the hierarchy
// by feeding it name and property data read from the source file.
class FactorySession : public Base {
 public:
  explicit FactorySession(const Factory &factory)
      : factory_(factory), root_(NULL) {}

  void BeginEntity(const char *name, const PropertyMap &properties);
  void EndEntity();

  Entity* RootEntity() { return root_; }
  Entity* CurrentEntity()
      { return entity_stack_.empty() ? NULL : entity_stack_.top(); }

 protected:
  const Factory &factory_;
  Stack<Entity*> entity_stack_;
  Entity *root_;

 private:  // Disallow copying
  FactorySession(const FactorySession&);
  void operator=(const FactorySession&);
};

// Subclasses of Parser implement using a particular file format and parsing
// engine to read dialog resource data, and passing it to a FactorySession.
class Parser : public Base {
 public:
  Parser() {}
  virtual ~Parser() {}

  virtual Entity* LoadEntityFromFile(const char *path) const { return NULL; }
  virtual Entity* LoadEntityFromData(const char *data) const { return NULL; }
};

}  // namespace Diadem

#endif  // DIADEM_FACTORY_H_
