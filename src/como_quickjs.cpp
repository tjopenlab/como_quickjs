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

static int initComoModule(const std::string &str_);
static JSCFunctionListEntry genComoProtoFuncs(MetaCoclass *metaCoclass);

static void *JS_GetRawOpaque(JSValueConst obj)
{
    JSObject *p;
    if (JS_VALUE_GET_TAG(obj) != JS_TAG_OBJECT)
        return NULL;
    return p->u.opaque;
}


static void js_como_finalizer(JSRuntime *rt, JSValue val)
{
    JSPointData *s = JS_GetOpaque(val, js_point_class_id);
    /* Note: 's' can be NULL in case JS_SetOpaque() was not called */
    js_free_rt(rt, s);
}

static JSValue js_como_ctor(JSContext *ctx,
                             JSValueConst new_target,
                             int argc, JSValueConst *argv,
                             int magic)
{
    JSValue obj = JS_UNDEFINED;
    JSValue proto;

    MetaCoclass *metacc = metaComponent->como_classes[magic];
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto))
        goto fail;
    MetaCoclass *metacc = JS_VALUE_GET_PTR(proto);

    ComoJsClassStub* stub = new ComoJsClassStub(ctx, metacc, thisObject);

    /* using new_target to get the prototype is necessary when the
       class is extended. */
    proto = JS_GetPropertyStr(ctx, new_target, "prototype");
    if (JS_IsException(proto))
        goto fail;
    obj = JS_NewObjectProtoClass(ctx, proto, js_point_class_id);
    JS_FreeValue(ctx, proto);
    if (JS_IsException(obj))
        goto fail;
    JS_SetOpaque(obj, stub);
    return obj;
 fail:
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
}

static JSValue js_como_norm(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv,
                             int magic)
{
    JSPointData *s = JS_GetRawOpaque(this_val);
    if (!s)
        return JS_EXCEPTION;
    return JS_NewFloat64(ctx, sqrt((double)s->x * s->x + (double)s->y * s->y));
}


static int js_como_init(JSContext *ctx, JSModuleDef *m)
{
    JSClassDef js_como_class = {
        "Como",
        .finalizer = js_como_finalizer,
    };

    JSCFunctionListEntry js_como_proto_funcs[] = {
        //JS_CGETSET_MAGIC_DEF("x", js_point_get_xy, js_point_set_xy, 0),
        //JS_CGETSET_MAGIC_DEF("y", js_point_get_xy, js_point_set_xy, 1),
        JS_CFUNC_MAGIC_DEF("norm", 0, js_point_norm, 0),
    };

    JSValue point_proto, point_class;
    const char *str_moduleName = JS_AtomToCString(ctx, m->module_name);

    metaComponent = new MetaComponent(ctx, str_moduleName);

    for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
        MetaCoclass *metaCoclass = metaComponent->como_classes[i];
        std::string className = metaCoclass->GetName();
        std::string classNs = metaCoclass->GetNamespace();

        Logger::V("como_jsbind", "load class, className: %s\n", className.c_str());
        ComoJsClassStub clz_ = ComoJsClassStub(m, className.c_str(),
                                                                       py::module_local());
        g_como_classes.insert(std::pair<std::string, ComoJsClassStub>(
                                                            classNs + "." + className, clz_));
        /* create the Point class */
        JS_NewClassID(&js_point_class_id);
        JS_NewClass(JS_GetRuntime(ctx), js_point_class_id, &js_como_class);

        js_como_proto_funcs = genComoProtoFuncs(metaCoclass);

        point_proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, point_proto, js_point_proto_funcs, countof(js_como_proto_funcs));

        // magic: which COMO Constructor
        int magic = 0;
        point_class = JS_NewCFunction2(ctx, js_point_ctor, "Point", 2, JS_CFUNC_constructor_magic, magic);
        /* set proto.constructor and ctor.prototype */
        JS_SetConstructor(ctx, point_class, point_proto);

        // store metaCoclass in property
        JSValue como_proto = JS_MKPTR(JS_TAG_SYMBOL, metaCoclass);
        JS_SetClassProto(ctx, js_point_class_id, como_proto);




//add a field??
struct JSClass {
    uint32_t class_id; /* 0 means free entry */
    JSAtom class_name;
    JSClassFinalizer *finalizer;
    JSClassGCMark *gc_mark;
    JSClassCall *call;
    /* pointers for exotic behavior, can be NULL if none are present */
    const JSClassExoticMethods *exotic;
};






        JS_SetModuleExport(ctx, m, "Point", point_class);
    }

    return 0;
}

void JS_AddComoModule(JSContext *ctx, const char *moduleName)
{
    int i = initComoModule(ctx, std::string(moduleName));

    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_como_init);
    if (!m)
        return NULL;

    std::map<std::string, ComoJsClassStub>::iterator iter;
    iter = g_como_classes.begin();
    if (iter != g_como_classes.end()) {
        JS_AddModuleExport(ctx, m, iter->first);
    }

    return m;
}

static JSCFunctionListEntry genComoProtoFuncs(MetaCoclass *metaCoclass)
{
    /*
    JSCFunctionListEntry js_como_proto_funcs[] = {
        //JS_CGETSET_MAGIC_DEF("x", js_point_get_xy, js_point_set_xy, 0),
        //JS_CGETSET_MAGIC_DEF("y", js_point_get_xy, js_point_set_xy, 1),
        JS_CFUNC_MAGIC_DEF("norm", 0, js_point_norm, 0),
    };
    */
    JSCFunctionListEntry *js_como_proto_funcs;
    js_como_proto_funcs = calloc(metaCoclass->methodNumber, sizeof(JSCFunctionListEntry));

    Array<IMetaMethod*> methods;
    char buf[MAX_METHOD_NAME_LENGTH];
    for (int j = 0;  j < metaCoclass->methodNumber; j++) {
        metaCoclass->GetMethodName(j, buf);
        Logger::V("como_quickjs", "load method, methodName: %s\n", buf);
        js_como_proto_funcs[i].name = strdup(buf);
    }
}


static py::module_ *this_pymodule = nullptr;
static MetaComponent *metaComponent = nullptr;
std::map<std::string, ComoJsClassStub> g_como_classes;

static int initComoModule(JSContext *ctx, const std::string &str_) {
    metaComponent = new MetaComponent(ctx, str_);

    for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
        MetaCoclass *metaCoclass = metaComponent->como_classes[i];
        std::string className = metaCoclass->GetName();
        std::string classNs = metaCoclass->GetNamespace();

        Logger::V("como_jsbind", "load class, className: %s\n", className.c_str());
        ComoJsClassStub clz_ = ComoJsClassStub(m, className.c_str(),
                                                                       py::module_local());
        g_como_classes.insert(std::pair<std::string, ComoJsClassStub>(
                                                            classNs + "." + className, clz_));

        switch (i) {

#define LAMBDA_FOR_CLASS_INIT(_NO_)                                                             \
            case _NO_:                                                                          \
                clz_.def(py::init([](py::args args, py::kwargs kwargs) {                        \
                    MetaCoclass *metacc = metaComponent->como_classes[_NO_];                    \
                    if (args.size() == 0) {                                                     \
                        AutoPtr<IInterface> thisObject = metacc->CreateObject();                \
                        if (thisObject == nullptr) {                                            \
                            std::string className = std::string(metacc->GetName());             \
                            throw std::runtime_error("initialize COMO class: " + className);    \
                        }                                                                       \
                        ComoJsClassStub* stub = new ComoJsClassStub(ctx, metacc, thisObject);   \
                        return stub;                                                            \
                    } else {                                                                    \
                        ComoJsClassStub* stub = new ComoJsClassStub(ctx, metacc);               \
                        metacc->constructObj(stub, args, kwargs);                               \
                        if (stub->thisObject == nullptr) {                                      \
                            std::string className = std::string(metacc->GetName());             \
                            throw std::runtime_error("initialize COMO class: " + className);    \
                        }                                                                       \
                        return stub;                                                            \
                    }                                                                           \
                }));                                                                            \
                break;

#include "LAMBDA_FOR_CLASS_INIT.inc"
#undef LAMBDA_FOR_CLASS_INIT
        }

        clz_.def("getAllConstants", &ComoJsClassStub::GetAllConstants);

        Array<IMetaMethod*> methods;
        char buf[MAX_METHOD_NAME_LENGTH];
        for (int j = 0;  j < metaCoclass->methodNumber; j++) {
            metaCoclass->GetMethodName(j, buf);
            Logger::V("como_pybind", "load method, methodName: %s\n", buf);
            switch (j) {

#define LAMBDA_FOR_METHOD(_NO_)                                         \
                case _NO_:                                      \
                    clz_.def(buf, &ComoJsClassStub::m##_NO_);   \
                    break;

#include "LAMBDA_FOR_METHOD.inc"
#undef LAMBDA_FOR_METHOD
            }
        }
    }
    return metaComponent;
}

