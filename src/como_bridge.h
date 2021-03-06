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

#ifndef __COMO_BRIDGE_H__
#define __COMO_BRIDGE_H__

#include <vector>
#include <comoapi.h>
#include "como_pytypes.h"

class MetaConstant;
class MetaType;
class MetaValue;
class MetaCoclass;

#define MAX_METHOD_NAME_LENGTH 1024
extern std::map<std::string, ComoJsObjectStub> g_como_classes;

// MetaComponent
///////////////////////////////

#pragma GCC visibility push(hidden)
// disable warning: ‘MetaComponent’ declared with greater visibility than the
// type of its field ‘MetaComponent::como_classes’ [-Wattributes]

class MetaComponent {
public:
    MetaComponent(JSContext *ctx_, const std::string &componentPath_)
                : ctx(ctx_)
                , componentPath(componentPath_) {
        String path(componentPath.c_str());
        CoGetComponentMetadataWithPath(path, nullptr, componentHandle);
        GetAllCoclasses();
    }

    MetaComponent(JSContext *ctx_, const std::string &componentPath_,
                                   AutoPtr<IMetaComponent> componentHandle_)
                : ctx(ctx_)
                , componentHandle(componentHandle_)
                , componentPath(componentPath_) {
        String path(componentPath.c_str());
        GetAllCoclasses();
    }

    ~MetaComponent() {
        Logger::V("como_quickjs", "delete MetaComponent object");
    }

    std::string GetName();
    std::string GetComponentID();
    std::map<std::string, JSValue> GetAllConstants();

    std::string componentPath;
    std::vector<MetaCoclass*> como_classes;
    std::vector<void*> vector_void_p;
private:
    JSContext *ctx;
    void GetAllCoclasses();
    AutoPtr<IMetaComponent> componentHandle;
};

#pragma GCC visibility pop

// MetaCoclass
///////////////////////////////
class MetaCoclass {
public:
    MetaCoclass(JSContext *ctx_, AutoPtr<IMetaCoclass> metaCoclass_)
            : ctx(ctx_)
            , metaCoclass(metaCoclass_) {
        metaCoclass_->GetMethodNumber(methodNumber);
        Array<IMetaMethod*> methods_(methodNumber);
        ECode ec = metaCoclass_->GetAllMethods(methods_);
        if (FAILED(ec)) {
            String str;
            metaCoclass_->GetName(str);
            std::string className = std::string(str.string());
            throw std::runtime_error("COMO class GetAllMethods: " + className);
        }
        methods = methods_;

        Array<Boolean> overridesInfo_(methodNumber);
        ec = metaCoclass_->GetAllMethodsOverrideInfo(overridesInfo_);
        if (FAILED(ec)) {
            String str;
            metaCoclass_->GetName(str);
            std::string className = std::string(str.string());
            throw std::runtime_error("COMO class GetAllMethodsOverrideInfo: " + className);
        }
        overridesInfo = overridesInfo_;

        metaCoclass_->GetConstructorNumber(constrsNumber);
        Array<IMetaConstructor*> constrs_(constrsNumber);
        ec = metaCoclass_->GetAllConstructors(constrs_);
        if (FAILED(ec)) {
            String str;
            metaCoclass_->GetName(str);
            std::string className = std::string(str.string());
            throw std::runtime_error("COMO class GetAllConstructors: " + className);
        }
        constrs = constrs_;
    }

    std::string GetName();
    std::string GetNamespace();
    void GetMethodName(int idxMethod, char *buf);
    int GetMethodParameterNumber(int idxMethod);
    AutoPtr<IInterface> CreateObject();
    void constructObj(ComoJsObjectStub *stub, int argc, JSValueConst *argv);

    Integer methodNumber;
    Integer constrsNumber;
    AutoPtr<IMetaCoclass> metaCoclass;
    Array<IMetaMethod*> methods;

private:
    JSContext *ctx;
    Array<IMetaConstructor*> constrs;
    Array<Boolean> overridesInfo;
};

#endif
