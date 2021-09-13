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

#ifndef __COMO_QUICKJS_H__
#define __COMO_QUICKJS_H__

#include "quickjs-libc.h"

void JS_AddComoModule(JSContext *ctx, const char *moduleName);

/* COMO
 * for JSObject(, JSContext) are defined in quickjs.c, these functions have to be
 * put in quickjs.c
 */
void *JS_GetRawOpaque(JSValueConst obj);
void JS_SetClassComoClass(JSContext *ctx, JSClassID class_id, void *metacc);
void *JS_GetClassComoClass(JSContext *ctx, JSClassID class_id);
JSClassID JS_GetJSObjectClassID(JSObject *obj);
const char *JS_GetModuleNameCString(JSContext *ctx, JSModuleDef *m);
void JS_SetJSModuleDefHdComo(JSModuleDef *m, void *hd);
void *JS_GetJSModuleDefHdComo(JSModuleDef *m);
/* COMO
 */

#endif //__COMO_QUICKJS_H__
