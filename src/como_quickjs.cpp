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

static JSCFunctionListEntry *genComoProtoFuncs(MetaCoclass *metaCoclass);

static void js_como_finalizer(JSRuntime *rt, JSValue val)
{
    ComoJsObjectStub *stub = (ComoJsObjectStub *)JS_GetRawOpaque(val);
    // Note: 'stub' can be NULL in case JS_SetOpaque() was not called
    // delete COMO object
}

static JSValue js_como_ctor(JSContext *ctx, JSValueConst new_target,
                            int argc, JSValueConst *argv)
{
    JSValue obj = JS_UNDEFINED;
    JSValue proto;

    JSObject *p;
    p = JS_VALUE_GET_OBJ(new_target);

    JSClassID class_id = JS_GetJSObjectClassID(p);

    MetaCoclass *metacc = (MetaCoclass *)JS_GetClassComoClass(ctx, class_id);
    ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metacc);

    if (argc == 0) {
        AutoPtr<IInterface> thisObject = metacc->CreateObject();
        if (thisObject == nullptr)
            goto fail;
        ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metacc, thisObject);
        if (stub == nullptr)
            goto fail;
    } else {
        ComoJsObjectStub* stub = new ComoJsObjectStub(ctx, metacc);
        if (stub == nullptr)
            goto fail;
        metacc->constructObj(stub, argc, argv);
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


static int js_como_init(JSContext *ctx, JSModuleDef *m)
{
    JSClassDef js_como_class = {
        "Como",
        .finalizer = js_como_finalizer,
    };

    JSClassID js_como_class_id;

    JSCFunctionListEntry *js_como_proto_funcs;

    JSValue como_proto, como_class;
    const char *str_moduleName = JS_GetModuleNameCString(ctx, m);

    MetaComponent *metaComponent = new MetaComponent(ctx, str_moduleName);

    for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
        MetaCoclass *metaCoclass = metaComponent->como_classes[i];
        std::string className = metaCoclass->GetName();
        std::string classNs = metaCoclass->GetNamespace();

        Logger::V("como_quickjs", "load class, className: %s\n", className.c_str());

        // create the Point class
        JS_NewClassID(&js_como_class_id);
        JS_NewClass(JS_GetRuntime(ctx), js_como_class_id, &js_como_class);

        js_como_proto_funcs = genComoProtoFuncs(metaCoclass);

        como_proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, como_proto, js_como_proto_funcs, metaCoclass->methodNumber);

        como_class = JS_NewCFunction2(ctx, js_como_ctor, className.c_str(), 2, JS_CFUNC_constructor, 0);

        // set proto.constructor and ctor.prototype
        JS_SetConstructor(ctx, como_class, como_proto);

        JS_SetClassComoClass(ctx, js_como_class_id, metaCoclass);

        // store metaCoclass in property
        JSValue como_proto = JS_MKPTR(JS_TAG_SYMBOL, metaCoclass);
        JS_SetClassProto(ctx, js_como_class_id, como_proto);

        JS_SetModuleExport(ctx, m, className.c_str(), como_class);
    }

    return 0;
}

void JS_AddComoModule(JSContext *ctx, const char *moduleName)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, moduleName, js_como_init);
    if (!m)
        return;

    std::map<std::string, ComoJsObjectStub>::iterator iter;
    iter = g_como_classes.begin();
    if (iter != g_como_classes.end()) {
        JS_AddModuleExport(ctx, m, iter->first.c_str());
    }
}

static JSCFunctionListEntry *genComoProtoFuncs(MetaCoclass *metaCoclass)
{
    JSCFunctionListEntry *js_como_proto_funcs;
    js_como_proto_funcs = (JSCFunctionListEntry *)calloc(metaCoclass->methodNumber, sizeof(JSCFunctionListEntry));

    JSCFunctionListEntry *jscfle;
    char buf[MAX_METHOD_NAME_LENGTH];
    for (int i = 0;  i < metaCoclass->methodNumber; i++) {
        metaCoclass->GetMethodName(i, buf);
        Logger::V("como_quickjs", "load method, methodName: %s\n", buf);
        jscfle = &js_como_proto_funcs[i];
        jscfle->name = strdup(buf);
        jscfle->prop_flags = JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE;
        jscfle->def_type = JS_DEF_CFUNC;
        jscfle->magic = i;
        jscfle->u.func.length = 0;
        jscfle->u.func.cproto = JS_CFUNC_generic_magic;
        jscfle->u.func.cfunc.generic_magic = js_como_method;
    }

    return js_como_proto_funcs;
}
