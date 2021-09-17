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

#include <comoapi.h>
#include "como_bridge.h"
#include "como_quickjs.h"
#include "utils.h"

extern "C" int JS_FindComoClass(JSContext *ctx, const char *className);

// MetaComponent
///////////////////////////////
std::string MetaComponent::GetName()
{
    String str;
    componentHandle->GetName(str);
    return std::string(str.string());
}

std::string MetaComponent::GetComponentID()
{
    String str;
    componentHandle->GetName(str);
    return std::string(str.string());
}

std::map<std::string, JSValue> MetaComponent::GetAllConstants()
{
    Integer constantNumber;
    componentHandle->GetConstantNumber(constantNumber);
    Array<IMetaConstant*> constants(constantNumber);
    componentHandle->GetAllConstants(constants);

    return constantsToMap(ctx, constants);
}

void MetaComponent::GetAllCoclasses()
{
    Integer number;
    componentHandle->GetCoclassNumber(number);
    Array<IMetaCoclass*> klasses(number);
    componentHandle->GetAllCoclasses(klasses);
    for (int i = 0;  i < number;  i++) {
        MetaCoclass *metaCoclass;
        metaCoclass = new MetaCoclass(ctx, klasses[i]);
        como_classes.push_back(metaCoclass);
    }
}

// MetaCoclass
///////////////////////////////
std::string MetaCoclass::GetName()
{
    String str;
    metaCoclass->GetName(str);
    return std::string(str.string());
}

std::string MetaCoclass::GetNamespace()
{
    String ns;
    metaCoclass->GetNamespace(ns);
    ns = ns.Replace("::", ".");
    return std::string(ns.string());
}

int MetaCoclass::GetMethodParameterNumber(int idxMethod)
{
    Integer number;
    methods[idxMethod]->GetParameterNumber(number);
    return number;
}

void MetaCoclass::GetMethodName(int idxMethod, char *buf)
{
    String str;
    methods[idxMethod]->GetName(str);

    buf[MAX_METHOD_NAME_LENGTH-1] = '\0';
    if (overridesInfo[idxMethod]) {
        String signature;
        methods[idxMethod]->GetSignature(signature);

        /* Replace all special signature character
            | Array       |     [     |
            | Pointer     |     *     |
            | Reference   |     &     |
            | Enum        | Lxx/xx;   |
            | Interface   | Lxx/xx;   |
        */
        signature = signature.Replace("[", "_0_")
                             .Replace("*", "_1_")
                             .Replace("&", "_2_")
                             .Replace("/", "_3_");

        strncpy(buf, (str+"__"+signature).string(), MAX_METHOD_NAME_LENGTH-1);
        return;
    }
    strncpy(buf, str.string(), MAX_METHOD_NAME_LENGTH-1);
}

AutoPtr<IInterface> MetaCoclass::CreateObject()
{
    AutoPtr<IInterface> object(nullptr);
    ECode ec = metaCoclass->CreateObject(IID_IInterface, &object);
    if (FAILED(ec)) {
        return nullptr;
    }
    return object;
}

void MetaCoclass::constructObj(ComoJsObjectStub* stub, int argc, JSValueConst *argv)
{
    if ((argc > 1) && JS_IsString(argv[0])) {
        const char *signature = JS_ToCString(ctx, argv[0]);
        AutoPtr<IMetaConstructor> constr;

        ECode ec = metaCoclass->GetConstructor(signature, constr);
        if (constr == nullptr) {
            std::string className = GetName();
            std::string classNs = GetNamespace();

            throw std::runtime_error("Can't construct object for "+classNs+"."+
                                     className+" with signature "+signature);
            stub->thisObject = nullptr;
            return;
        }
        stub->methodimpl(constr, argc-1, &argv[1], true);
    }
    else {
        Integer number;
        Integer constrNumber = constrs.GetLength();
        for (Integer i = 0;  i < constrNumber;  i++) {
            IMetaConstructor *constr = constrs[i];
            constr->GetParameterNumber(number);
            if (number == argc) {
                stub->methodimpl(constr, argc, argv, true);
                return;
            }
        }
        std::string className = GetName();
        std::string classNs = GetNamespace();
        throw std::runtime_error("Can't construct object for "+classNs+"."+className+
                                 " with "+std::to_string(argc)+" parameters");
        stub->thisObject = nullptr;
    }
}

// ComoJsObjectStub
///////////////////////////////
ComoJsObjectStub::ComoJsObjectStub(JSContext *ctx_, MetaCoclass *mCoclass)
    : ctx(ctx_)
    , thisObject(nullptr),
    methods(mCoclass->methods)
{}

ComoJsObjectStub::ComoJsObjectStub(JSContext *ctx_, MetaCoclass *mCoclass, AutoPtr<IInterface> thisObject_)
    : ctx(ctx_)
    , thisObject(thisObject_),
    methods(mCoclass->methods)
{}


std::map<std::string, JSValue> ComoJsObjectStub::GetAllConstants()
{
    std::map<std::string, JSValue> out;

    AutoPtr<IMetaCoclass> metaCoclass;
    IObject::Probe(thisObject)->GetCoclass(metaCoclass);

    Integer interfaceNumber;
    metaCoclass->GetInterfaceNumber(interfaceNumber);
    Array<IMetaInterface*> interfaces(interfaceNumber);
    metaCoclass->GetAllInterfaces(interfaces);
    for (Integer i = 0; i < interfaces.GetLength(); i++) {
        Integer constantNumber;
        interfaces[i]->GetConstantNumber(constantNumber);
        if (constantNumber > 0) {
            std::map<std::string, JSValue> out_;

            Array<IMetaConstant*> constants(constantNumber);
            interfaces[i]->GetAllConstants(constants);
            out_ = constantsToMap(ctx, constants);

            out.insert(out_.begin(), out_.end());
        }
    }
    return out;
}

JSValue ComoJsObjectStub::methodimpl(IMetaMethod *method, int argc, JSValueConst *argv, bool isConstructor)
{
    ECode ec = 0;
    AutoPtr<IArgumentList> argList;
    Boolean outArgs;
    Integer paramNumber;
    method->HasOutArguments(outArgs);
    method->GetParameterNumber(paramNumber);

    JSValue out_JSValue;
    HANDLE *outResult = nullptr;
    if (outArgs) {
        outResult = (HANDLE*)calloc(sizeof(HANDLE), paramNumber);
    }

    std::vector<std::string> signatureBreak;
    String signature;

    method->CreateArgumentList(argList);
    Array<IMetaParameter*> params(paramNumber);
    method->GetAllParameters(params);

    Integer inParam;
    if (isConstructor)
        inParam = 1;
    else
        inParam = 0;

    for (Integer i = 0; i < paramNumber; i++) {
        IMetaParameter* param = params[i];

        String name;
        Integer index;
        IOAttribute attr;
        AutoPtr<IMetaType> type;
        String tname;
        TypeKind kind;
        int iValue;
        int64_t lValue;
        bool bValue;
        double dValue;
        char *sValue;

        param->GetName(name);
        param->GetIndex(index);
        param->GetIOAttribute(attr);
        param->GetType(type);
        type->GetName(tname);
        type->GetTypeKind(kind);

        if (attr == IOAttribute::IN) {
            if (inParam >= argc) {
                // too much COMO input paramter
                ec = E_ILLEGAL_ARGUMENT_EXCEPTION;
                break;
            }
            switch (kind) {
                case TypeKind::Byte:
                    if (JS_ToInt32(ctx, &iValue, argv[inParam++]))
                        iValue = -1;

                    argList->SetInputArgumentOfByte(i, iValue);
                    break;
                case TypeKind::Short:
                    if (JS_ToInt32(ctx, &iValue, argv[inParam++]))
                        iValue = -1;

                    argList->SetInputArgumentOfShort(i, iValue);
                    break;
                case TypeKind::Integer:
                    if (JS_ToInt32(ctx, &iValue, argv[inParam++]))
                        iValue = -1;

                    argList->SetInputArgumentOfInteger(i, iValue);
                    break;
                case TypeKind::Long:
                    if (JS_ToInt64(ctx, &lValue, argv[inParam++]))
                        lValue = -1;

                    argList->SetInputArgumentOfLong(i, lValue);
                    break;
                case TypeKind::Float:
                    if (JS_ToFloat64(ctx, &dValue, argv[inParam++]))
                        dValue = -1;

                    argList->SetInputArgumentOfFloat(i, dValue);
                    break;
                case TypeKind::Double:
                    if (JS_ToFloat64(ctx, &dValue, argv[inParam++]))
                        dValue = -1;

                    argList->SetInputArgumentOfFloat(i, dValue);
                    break;
                case TypeKind::Char:
                    if (JS_ToInt32(ctx, &iValue, argv[inParam++]))
                        iValue = -1;

                    argList->SetInputArgumentOfChar(i, (Char)iValue);
                    break;
                case TypeKind::Boolean:
                    bValue = JS_ToBool(ctx, argv[inParam++]);

                    argList->SetInputArgumentOfChar(i, bValue);
                    break;
                case TypeKind::String:
                    argList->SetInputArgumentOfString(i, JS_ToCString(ctx, argv[inParam++]));
                    break;
                case TypeKind::Interface: {
                    if (JS_ToInt64(ctx, &lValue, argv[inParam++]))
                        lValue = -1;
                    ComoJsObjectStub* argObj = reinterpret_cast<ComoJsObjectStub *>(lValue);
                    argList->SetInputArgumentOfInterface(i, argObj->thisObject);
                    break;
                }
                case TypeKind::HANDLE:
                case TypeKind::CoclassID:
                case TypeKind::ComponentID:
                case TypeKind::InterfaceID:
                case TypeKind::Unknown:
                    inParam++;
                    break;
            }
        }
        else /*if (attr == IOAttribute::OUT)*/ {
            switch (kind) {
                case TypeKind::Byte:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Byte)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Short:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Short)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Integer:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Integer)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Long:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Long)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Float:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Float)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Double:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Double)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Char:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Char)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::Boolean:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(Boolean)));
                    argList->SetOutputArgumentOfInteger(i, outResult[i]);
                    break;
                case TypeKind::String:
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(String)));
                    argList->SetOutputArgumentOfString(i, outResult[i]);
                    break;
                case TypeKind::Interface: {
                    if (signatureBreak.empty()) {
                        method->GetSignature(signature);
                        breakSignature(signature, signatureBreak);
                    }

                    int class_id = JS_FindComoClass(ctx, signatureBreak[i].c_str());
                    if (class_id >= 0) {
                        outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(IInterface*)));
                        argList->SetOutputArgumentOfInterface(i, outResult[i]);
                    }
                    else {
                        throw std::runtime_error("no COMO class: " + name);
                    }
                    outResult[i] = reinterpret_cast<HANDLE>(malloc(sizeof(IInterface*)));
                    argList->SetOutputArgumentOfInterface(i, outResult[i]);
                    break;
                }
                case TypeKind::HANDLE:
                case TypeKind::CoclassID:
                case TypeKind::ComponentID:
                case TypeKind::InterfaceID:
                case TypeKind::Unknown:
                    break;
            }
        }
    }

    if (isConstructor) {
        ec = (reinterpret_cast<IMetaConstructor*>(method))->CreateObject(argList, thisObject);
        return out_JSValue;
    }

    if (ec == 0) {
        ec = method->Invoke(thisObject, argList);
    }

    if (outArgs && (outResult != nullptr)) {
        // collect output results into out_JSValue
        for (Integer i = 0; i < paramNumber; i++) {
            IMetaParameter* param = params[i];

            IOAttribute attr;
            AutoPtr<IMetaType> type;
            TypeKind kind;

            param->GetIOAttribute(attr);
            param->GetType(type);
            type->GetTypeKind(kind);

            if (attr == IOAttribute::OUT) {
                switch (kind) {
                    case TypeKind::Byte:
                        out_JSValue =  JS_NewInt32(ctx, *(reinterpret_cast<Byte*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Short:
                        out_JSValue = JS_NewInt32(ctx, *(reinterpret_cast<Short*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Integer:
                        out_JSValue = JS_NewInt32(ctx, *(reinterpret_cast<int*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Long:
                        out_JSValue = JS_NewInt64(ctx, *(reinterpret_cast<Long*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Float:
                        out_JSValue = JS_NewFloat64(ctx, (double)*(reinterpret_cast<Float*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Double:
                        out_JSValue = JS_NewFloat64(ctx, *(reinterpret_cast<Double*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Char:
                        out_JSValue = JS_NewInt32(ctx, *(reinterpret_cast<Char*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Boolean:
                        out_JSValue = JS_NewBool(ctx, *(reinterpret_cast<Boolean*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::String:
                        out_JSValue = JS_NewString(ctx, (*reinterpret_cast<String*>(outResult[i])).string());
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Interface: {
                        AutoPtr<IMetaCoclass> mCoclass_;
                        String name, ns;

                        AutoPtr<IInterface> thisObject_ = reinterpret_cast<IInterface*>(outResult[i]);
                        IObject::Probe(thisObject_)->GetCoclass(mCoclass_);
                        mCoclass_->GetName(name);
                        mCoclass_->GetNamespace(ns);

                        int class_id = JS_FindComoClass(ctx, name.string());
                        if (class_id >= 0) {
                            out_JSValue = js_box_JSValue(ctx, class_id, thisObject_);
                        }

                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    }
                    case TypeKind::HANDLE:
                    case TypeKind::CoclassID:
                    case TypeKind::ComponentID:
                    case TypeKind::InterfaceID:
                    case TypeKind::Unknown:
                        break;
                }
            }
        }

        free(outResult);
    }
    else {
        //out_JSValue = py::make_tuple(ec);
    }

    if (! signatureBreak.empty()) {
        signatureBreak.clear();
        signatureBreak.shrink_to_fit();
    }

    return out_JSValue;
}
