// Copyright 2011 Google Inc.
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

#include "stdafx.h"
#include <atlbase.h>
#include <xmllite.h>

#include "Diadem/XmlLiteParser.h"
#include "Diadem/Factory.h"
#include "Diadem/Value.h"


namespace {

// XML node and attribute names will be plain ASCII, so it's safe to cast
// WCHAR to char.
Diadem::String StringFromWCHARs(const WCHAR *w) {
  const size_t length = wcslen(w);
  char *s = new char[length + 1];

  for (int i = 0; w[i] != 0; ++i)
    s[i] = static_cast<char>(w[i]);
  s[length] = '\0';
  return Diadem::String(s, Diadem::String::kAdoptBuffer);
}

}

namespace Diadem {

static Entity* ParseStream(CComPtr<IStream> &stream, const Factory &factory) {
  CComPtr<IXmlReader> reader;

  if (FAILED(CreateXmlReader(
      __uuidof(IXmlReader), reinterpret_cast<void**>(&reader), NULL)))
    return NULL;
  if (FAILED(reader->SetInput(stream)))
    return NULL;
  reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit);

  XmlNodeType node_type;
  FactorySession session(factory);
  HRESULT result;

  while ((result = reader->Read(&node_type)) == S_OK) {
    switch (node_type) {
      case XmlNodeType_Element: {
        const WCHAR *node_name = NULL;

        if (FAILED(reader->GetLocalName(&node_name, NULL)))
          break;
        if (FAILED(reader->MoveToFirstAttribute()))
          break;

        PropertyMap properties;

        while (reader->MoveToNextAttribute() == S_OK) {
          const WCHAR *name = NULL, *value = NULL;

          if (FAILED(reader->GetLocalName(&name, NULL)))
            continue;
          if (FAILED(reader->GetValue(&value, NULL)))
            continue;
          properties.Insert(StringFromWCHARs(name), StringFromWCHARs(value));
        }
        session.BeginEntity(StringFromWCHARs(node_name), properties);
        break;
      }
      case XmlNodeType_EndElement:
        session.EndEntity();
        break;
    }
  }
  return session.RootEntity();
}

Entity* XmlLiteParser::LoadEntityFromFile(const char *path) const {
  CComPtr<IStream> file_stream;

  if (FAILED(SHCreateStreamOnFileA(path, STGM_READ, &file_stream)))
    return NULL;
  return ParseStream(file_stream, factory_);
}

Entity* XmlLiteParser::LoadEntityFromData(const char *data) const {
  CComPtr<IStream> data_stream;

  if (FAILED(CreateStreamOnHGlobal(NULL, true, &data_stream)))
    return NULL;
  if (FAILED(data_stream->Write(data, strlen(data), NULL)))
    return NULL;

  LARGE_INTEGER large;

  large.QuadPart = 0;
  data_stream->Seek(large, STREAM_SEEK_SET, NULL);
  return ParseStream(data_stream, factory_);
}

}  // namespace Diadem
