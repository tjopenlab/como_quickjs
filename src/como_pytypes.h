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

#include <comoapi.h>

class MetaCoclass;

class ComoPyClassStub {
public:
    ComoPyClassStub(MetaCoclass *mCoclass);
    ComoPyClassStub(MetaCoclass *mCoclass, AutoPtr<IInterface> thisObject_);

#define LAMBDA_FOR_METHOD(_NO_)                                 \
    py::tuple m##_NO_(py::args args, py::kwargs kwargs) {       \
        return methodimpl(methods[_NO_], args, kwargs, false);  \
    }

#include "LAMBDA_FOR_METHOD.inc"
#undef LAMBDA_FOR_METHOD

    std::map<std::string, py::object> GetAllConstants();
    py::tuple methodimpl(IMetaMethod *method, py::args args, py::kwargs kwargs, bool isConstructor);
    void refreshThisObject(AutoPtr<IMetaCoclass> mCoclass);

    AutoPtr<IInterface> thisObject;
    std::string className;

private:
    Array<IMetaMethod*> methods;
};

#endif
