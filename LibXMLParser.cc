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

#include "Diadem/LibXMLParser.h"

#include <libxml/parser.h>

#include "Diadem/Factory.h"
#include "Diadem/Value.h"

namespace Diadem {

static void ProcessXMLElement(
    xmlNode *element, FactorySession &session) {
  if (element->type != XML_ELEMENT_NODE)
    return;

  PropertyMap properties;

  for (xmlAttr *attr = element->properties; attr != NULL; attr = attr->next)
    properties.Insert(
        (const char*)attr->name,
        (const char*)attr->children->content);

  session.BeginEntity((const char*)element->name, properties);
  for (xmlNode *child = element->children; child != NULL; child = child->next)
    ProcessXMLElement(child, session);
  session.EndEntity();
}

static Entity* ProcessDocument(
    xmlDocPtr document, const Factory &factory) {
  if (document == NULL)
    return NULL;

  FactorySession session(factory);

  ProcessXMLElement(xmlDocGetRootElement(document), session);
  xmlFreeDoc(document);
  return session.RootEntity();
}

Entity* LibXMLParser::LoadEntityFromFile(const char *path) const {
  return ProcessDocument(xmlParseFile(path), factory_);
}

Entity* LibXMLParser::LoadEntityFromData(const char *data) const {
  return ProcessDocument(xmlParseMemory(data, strlen(data)), factory_);
}

}  // namespace Diadem
