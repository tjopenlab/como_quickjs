//=========================================================================
// Copyright (C) 2021 The C++ Component Model(COMO) Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//=========================================================================

#ifndef __COMO_PYTYPES_H__
#define __COMO_PYTYPES_H__

#include <map>
#include <comoapi.h>
#include "quickjs.h"

class MetaCoclass;

class ComoJsObjectStub {
public:
    ComoJsObjectStub(JSContext *ctx_, MetaCoclass *mCoclass);
    ComoJsObjectStub(JSContext *ctx_, MetaCoclass *mCoclass, AutoPtr<IInterface> thisObject_);

    std::map<std::string, JSValue> GetAllConstants();
    JSValue methodimpl(IMetaMethod *method, int argc, JSValueConst *argv, bool isConstructor);
    void refreshThisObject(AutoPtr<IMetaCoclass> mCoclass);

    AutoPtr<IInterface> thisObject;
    std::string className;
    Array<IMetaMethod*> methods;

private:
    JSContext *ctx;
};

#endif
