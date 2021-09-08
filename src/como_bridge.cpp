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
#include "utils.h"

// MetaComponent
///////////////////////////////
std::string MetaComponent::GetName() {
    String str;
    componentHandle->GetName(str);
    return std::string(str.string());
}

std::string MetaComponent::GetComponentID() {
    String str;
    componentHandle->GetName(str);
    return std::string(str.string());
}

std::map<std::string, py::object> MetaComponent::GetAllConstants() {
    Integer constantNumber;
    componentHandle->GetConstantNumber(constantNumber);
    Array<IMetaConstant*> constants(constantNumber);
    componentHandle->GetAllConstants(constants);

    return constantsToMap(constants);
}

void MetaComponent::GetAllCoclasses() {
    Integer number;
    componentHandle->GetCoclassNumber(number);
    Array<IMetaCoclass*> klasses(number);
    componentHandle->GetAllCoclasses(klasses);
    for (int i = 0;  i < number;  i++) {
        MetaCoclass *metaCoclass;
        metaCoclass = new MetaCoclass(klasses[i]);
        como_classes.push_back(metaCoclass);
    }
}

// MetaCoclass
///////////////////////////////
std::string MetaCoclass::GetName() {
    String str;
    metaCoclass->GetName(str);
    return std::string(str.string());
}

std::string MetaCoclass::GetNamespace() {
    String ns;
    metaCoclass->GetNamespace(ns);
    ns = ns.Replace("::", ".");
    return std::string(ns.string());
}

void MetaCoclass::GetMethodName(int idxMethod, char *buf) {
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

AutoPtr<IInterface> MetaCoclass::CreateObject() {
    AutoPtr<IInterface> object(nullptr);
    ECode ec = metaCoclass->CreateObject(IID_IInterface, &object);
    if (FAILED(ec)) {
        return nullptr;
    }
    return object;
}

void MetaCoclass::constructObj(ComoPyClassStub* stub, py::args args, py::kwargs kwargs)
{
    const char *signature = std::string(py::str(args[0])).c_str();
    AutoPtr<IMetaConstructor> constr;

    ECode ec = metaCoclass->GetConstructor(signature, constr);
    if (constr == nullptr) {
        std::string className = GetName();
        std::string classNs = GetNamespace();

        throw std::runtime_error("Can't construct object for " + classNs + "." + className + " with signature " + signature);
        stub->thisObject = nullptr;
        return;
    }
    stub->methodimpl(constr, args, kwargs, true);
}

// ComoPyClassStub
///////////////////////////////
ComoPyClassStub::ComoPyClassStub(MetaCoclass *mCoclass)
    : thisObject(nullptr),
    methods(mCoclass->methods)
{}

ComoPyClassStub::ComoPyClassStub(MetaCoclass *mCoclass, AutoPtr<IInterface> thisObject_)
    : thisObject(thisObject_),
    methods(mCoclass->methods)
{}


std::map<std::string, py::object> ComoPyClassStub::GetAllConstants() {
    std::map<std::string, py::object> out;

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
            std::map<std::string, py::object> out_;

            Array<IMetaConstant*> constants(constantNumber);
            interfaces[i]->GetAllConstants(constants);
            out_ = constantsToMap(constants);

            out.insert(out_.begin(), out_.end());
        }
    }
    return out;
}

/*
The class py::args derives from py::tuple and py::kwargs derives from py::dict.
Python types available wrappers https://pybind11.readthedocs.io/en/stable/advanced/pycpp/object.html

py::args args[] operator
    // https://docs.python.org/3/c-api/tuple.html
    PyObject* PyTuple_GetItem(PyObject *p, Py_ssize_t pos)
    Return value: Borrowed reference.
    Return the object at position pos in the tuple pointed to by p. If pos is out of bounds, return
    NULL and set an IndexError exception.
*/
py::tuple ComoPyClassStub::methodimpl(IMetaMethod *method, py::args args, py::kwargs kwargs, bool isConstructor) {
    ECode ec = 0;
    AutoPtr<IArgumentList> argList;
    Boolean outArgs;
    Integer paramNumber;
    method->HasOutArguments(outArgs);
    method->GetParameterNumber(paramNumber);

    py::tuple out_tuple;
    HANDLE *outResult = nullptr;
    if (outArgs) {
        py::tuple out_tuple = py::make_tuple();
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

        param->GetName(name);
        param->GetIndex(index);
        param->GetIOAttribute(attr);
        param->GetType(type);
        type->GetName(tname);
        type->GetTypeKind(kind);

        if (attr == IOAttribute::IN) {
            if (inParam >= args.size()) {
                // too much COMO input paramter
                ec = E_ILLEGAL_ARGUMENT_EXCEPTION;
                break;
            }
            switch (kind) {
                case TypeKind::Byte:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfByte(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfByte(i, int(py::int_(args[inParam++])));
                    break;
                case TypeKind::Short:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfShort(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfShort(i, int(py::int_(args[inParam++])));
                    break;
                case TypeKind::Integer:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfInteger(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfInteger(i, int(py::int_(args[inParam++])));
                    break;
                case TypeKind::Long:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfLong(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfLong(i, int(py::int_(args[inParam++])));
                    break;
                case TypeKind::Float:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfFloat(i, float(py::float_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfFloat(i, float(py::float_(args[inParam++])));
                    break;
                case TypeKind::Double:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfDouble(i, float(py::float_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfDouble(i, float(py::float_(args[inParam++])));
                    break;
                case TypeKind::Char:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfChar(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfChar(i, int(py::int_(args[inParam++])));
                    break;
                case TypeKind::Boolean:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfBoolean(i, int(py::int_(kwargs[name.string()])));
                    else
                        argList->SetInputArgumentOfBoolean(i, Boolean(py::bool_(args[inParam++])));
                    break;
                case TypeKind::String:
                    if (kwargs.contains(name.string()))
                        argList->SetInputArgumentOfString(i, String(std::string(py::str(kwargs[name.string()])).c_str()));
                    else
                        argList->SetInputArgumentOfString(i, String(std::string(py::str(args[inParam++])).c_str()));
                    break;
                case TypeKind::Interface: {
                    ComoPyClassStub* argObj = args[inParam++].cast<ComoPyClassStub*>();
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
                    AutoPtr<IMetaCoclass> mCoclass_;
                    String name, ns;
                    std::map<std::string, ComoPyClassStub>::iterator iter;

                    if (signatureBreak.empty()) {
                        method->GetSignature(signature);
                        breakSignature(signature, signatureBreak);
                    }

                    iter = g_como_classes.find(signatureBreak[i]);
                    if (iter != g_como_classes.end()) {
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
        return py::make_tuple();
    }

    if (ec == 0) {
        ec = method->Invoke(thisObject, argList);
    }

    if (outArgs && (outResult != nullptr)) {
        // collect output results into out_tuple
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
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Byte*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Short:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Short*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Integer:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<int*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Long:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Long*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Float:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Float*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Double:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Double*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Char:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Char*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Boolean:
                        out_tuple = py::make_tuple(out_tuple, *(reinterpret_cast<Boolean*>(outResult[i])));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::String:
                        out_tuple = py::make_tuple(out_tuple, std::string((*reinterpret_cast<String*>(outResult[i])).string()));
                        free(reinterpret_cast<void*>(outResult[i]));
                        break;
                    case TypeKind::Interface: {
                        AutoPtr<IMetaCoclass> mCoclass_;
                        String name, ns;
                        std::map<std::string, ComoPyClassStub>::iterator iter;

                        AutoPtr<IInterface> thisObject_ = reinterpret_cast<IInterface*>(outResult[i]);
                        IObject::Probe(thisObject_)->GetCoclass(mCoclass_);
                        mCoclass_->GetName(name);
                        mCoclass_->GetNamespace(ns);
                        iter = g_como_classes.find((ns + "." + name).string());
                        if (iter != g_como_classes.end()) {
                            ComoPyClassStub py_cls = iter->second;
                            py::object py_obj = py_cls();
                            out_tuple = py::make_tuple(out_tuple, py_obj);
                        }
                        else {
                            out_tuple = py::make_tuple(out_tuple, py::none());
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
        out_tuple = py::make_tuple(ec);
    }

    if (! signatureBreak.empty()) {
        signatureBreak.clear();
        signatureBreak.shrink_to_fit();
    }

    return out_tuple;
}
