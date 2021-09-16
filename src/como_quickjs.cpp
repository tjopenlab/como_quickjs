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

using namespace como;

static JSCFunctionListEntry *genComoProtoFuncs(JSContext *ctx, MetaCoclass *metaCoclass);

static void js_como_finalizer(JSRuntime *rt, JSValue val)
{
    ComoJsObjectStub *stub = (ComoJsObjectStub *)JS_GetRawOpaque(val);
    // Note: 'stub' can be NULL in case JS_SetOpaque() was not called
    if (stub != nullptr) {
        delete stub;
    }
}

static JSValue js_como_ctor(JSContext *ctx, JSValueConst new_target,
                            int argc, JSValueConst *argv)
{
    JSValue obj = JS_UNDEFINED;
    JSValue proto;

    JSObject *p;
    p = JS_VALUE_GET_OBJ(new_target);

    JSClassID class_id = JS_GetJSObjectClassID(p);

    MetaCoclass *metaCoclass = (MetaCoclass *)JS_GetClassComoClass(ctx, class_id);
    ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metaCoclass);

    if (argc == 0) {
        AutoPtr<IInterface> thisObject = metaCoclass->CreateObject();
        if (thisObject == nullptr)
            goto fail;
        ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metaCoclass, thisObject);
        if (stub == nullptr)
            goto fail;
    } else {
        ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metaCoclass);
        if (stub == nullptr)
            goto fail;
        metaCoclass->constructObj(stub, argc, argv);
    }

    // using new_target to get the prototype is necessary when the
    // class is extended.
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto))
        goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj))
        goto fail;
    JS_SetOpaque(obj, stub);
    return obj;
 fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static JSValue js_como_method(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv,
                              int magic)
{
    ComoJsObjectStub *stub = (ComoJsObjectStub *)JS_GetRawOpaque(this_val);
    if (!stub)
        return JS_EXCEPTION;
    return stub->methodimpl(stub->methods[magic], argc, argv, false);
}

extern "C" int js_exportComoClasses(JSContext *ctx, JSModuleDef *m, const char *module_name, void *hd)
{
    AutoPtr<IMetaComponent> mc;
    ECode ec = CoGetComponentMetadataFromFile(reinterpret_cast<HANDLE>(hd), nullptr, mc);
    MetaComponent *metaComponent = new MetaComponent(ctx, module_name, mc);

    //TODO, temp code
    LoggerSetLevel();

    for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
        MetaCoclass *metaCoclass = metaComponent->como_classes[i];
        std::string className = metaCoclass->GetName();
        JS_AddModuleExport(ctx, m, className.c_str());
    }

    JS_SetJSModuleDefMetaComponent(m, metaComponent);

    return 0;
}

extern "C" int js_como_init(JSContext *ctx, JSModuleDef *m)
{
    JSClassDef js_como_class = {
        .finalizer = js_como_finalizer,
    };

    JSCFunctionListEntry *js_como_proto_funcs;
    JSValue como_proto, como_class;
    JSClassID class_id;
    MetaComponent *metaComponent = (MetaComponent *)JS_GetJSModuleDefMetaComponent(m);
    if (metaComponent == nullptr)
        return 0;

    for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
        MetaCoclass *metaCoclass = metaComponent->como_classes[i];
        std::string className = metaCoclass->GetName();
        std::string classNs = metaCoclass->GetNamespace();
        const char *szClassName = className.c_str();

        Logger::V("como_quickjs", "load class, className: %s\n", szClassName);

        class_id = 0;
        JS_NewClassID(&class_id);
        js_como_class.class_name = szClassName;
        JS_NewClass(JS_GetRuntime(ctx), class_id, &js_como_class);

        js_como_proto_funcs = genComoProtoFuncs(ctx, metaCoclass);
        como_proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, como_proto, js_como_proto_funcs, metaCoclass->methodNumber);

        // int arg_count = p->u.cfunc.length;
        const int arg_count = 0;
        como_class = JS_NewCFunction2(ctx, js_como_ctor, szClassName, arg_count, JS_CFUNC_constructor, 0);

        // set proto.constructor and ctor.prototype
        JS_SetConstructor(ctx, como_class, como_proto);
        JS_SetClassProto(ctx, class_id, como_proto);

        JS_SetClassComoClass(ctx, class_id, metaCoclass);

        JS_SetModuleExport(ctx, m, szClassName, como_class);
    }

    return 0;
}

static JSCFunctionListEntry *genComoProtoFuncs(JSContext *ctx, MetaCoclass *metaCoclass)
{
    JSCFunctionListEntry *js_como_proto_funcs;
    js_como_proto_funcs = (JSCFunctionListEntry *)calloc(metaCoclass->methodNumber, sizeof(JSCFunctionListEntry));
    if (js_como_proto_funcs == nullptr)
        return nullptr;

    JSCFunctionListEntry *jscfle;
    char buf[MAX_METHOD_NAME_LENGTH];
    for (int i = 0;  i < metaCoclass->methodNumber; i++) {
        metaCoclass->GetMethodName(i, buf);
        Logger::V("como_quickjs", "load method, methodName: %s\n", buf);
        jscfle = &js_como_proto_funcs[i];

        //jscfle->name = strdup(buf);         // TODO, when to free it?
        jscfle->name = JS_AtomToCString(ctx, JS_NewAtom(ctx, buf)); // TODO: Atom leaks

        jscfle->prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
        jscfle->def_type = JS_DEF_CFUNC;
        jscfle->magic = i;
        jscfle->u.func.length = metaCoclass->GetMethodParameterNumber(i);
        jscfle->u.func.cproto = JS_CFUNC_generic_magic;
        jscfle->u.func.cfunc.generic_magic = js_como_method;
    }

    return js_como_proto_funcs;
}

extern "C" void freeMetaComponent(void *metaComponent)
{
    delete (MetaComponent *)metaComponent;
}

void LoggerSetLevel()
{
    Logger::SetLevel(Logger::VERBOSE);
}
