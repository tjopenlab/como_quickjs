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
#include <cassert>
#include "utils.h"

std::map<std::string, JSValue> constantsToMap(JSContext *ctx, Array<IMetaConstant*> &constants) {
    std::map<std::string, JSValue> out;

    for (Integer i = 0; i < constants.GetLength(); i++) {
        String name, ns;
        constants[i]->GetName(name);
        constants[i]->GetNamespace(ns);
        if (! ns.IsEmpty()) {
            ns = ns.Replace("::", ".");
            if (! ns.EndsWith("."))
                ns = ns + ".";
        }
        std::string strName((ns + name).string());

        AutoPtr<IMetaType> type;
        constants[i]->GetType(type);
        AutoPtr<IMetaValue> value;
        constants[i]->GetValue(value);
        TypeKind kind;
        type->GetTypeKind(kind);

        switch (kind) {
            case TypeKind::Byte: {
                Byte byte;
                value->GetByteValue(byte);
                out.insert({strName, JS_NewInt32(ctx, byte)});
                break;
            }
            case TypeKind::Short: {
                Short svalue;
                value->GetShortValue(svalue);
                out.insert({strName, JS_NewInt32(ctx, svalue)});
                break;
            }
            case TypeKind::Integer: {
                Integer ivalue;
                value->GetIntegerValue(ivalue);
                out.insert({strName, JS_NewInt32(ctx, ivalue)});
                break;
            }
            case TypeKind::Long: {
                Long lvalue;
                value->GetLongValue(lvalue);
                out.insert({strName, JS_NewInt64(ctx, lvalue)});
                break;
            }
            case TypeKind::Float: {
                Float fvalue;
                value->GetFloatValue(fvalue);
                out.insert({strName, JS_NewFloat64(ctx, (double)fvalue)});
                break;
            }
            case TypeKind::Double: {
                Double dvalue;
                value->GetDoubleValue(dvalue);
                out.insert({strName, JS_NewFloat64(ctx, dvalue)});
                break;
            }
            case TypeKind::Char: {
                Char cvalue;
                value->GetCharValue(cvalue);
                out.insert({strName, JS_NewInt32(ctx, cvalue)});
                break;
            }
            case TypeKind::Boolean: {
                Boolean b;
                value->GetBooleanValue(b);
                out.insert({strName, JS_NewBool(ctx, b)});
                break;
            }
            case TypeKind::String: {
                String str;
                value->GetStringValue(str);
                out.insert({strName, JS_NewString(ctx, str)});
                break;
            }
            case TypeKind::HANDLE:
            case TypeKind::CoclassID:
            case TypeKind::ComponentID:
            case TypeKind::InterfaceID:
            case TypeKind::Interface:
            case TypeKind::Unknown:
                break;
        }
    }

    return out;
}

/********
    | Type        | Signature |
    |:-----------:|:---------:|
    | Byte        |     B     |
    | Short       |     S     |
    | Integer     |     I     |
    | Long        |     J     |
    | Float       |     F     |
    | Double      |     D     |
    | Char        |     C     |
    | Boolean     |     Z     |
    | String      |     T     |
    | HANDLE      |     H     |
    | ECode       |     E     |
    | CoclassID   |     K     |
    | ComponentID |     M     |
    | InterfaceID |     U     |
    | Triple      |     R     |
    | Array       |     [     |
    | Pointer     |     *     |
    | Reference   |     &     |
    | Enum        | Lxx/xx;   |
    | Interface   | Lxx/xx;   |
*********/
int getOneType(const char *str, char *buf);
void breakSignature(String &signature, std::vector<std::string> &signatureVector) {
    const char *str = signature.string();
    char c = *str;
    char buf[256];

    while (c != '\0') {
        int len = getOneType(str, buf);
        if (len == 0)
            break;
        std::string str_ = std::string(buf);
        signatureVector.push_back(str_);
        str += len;
        c = *str;
    }
}

int getOneType(const char *str, char *buf) {
    char c = *str;
    if (c != '\0') {
        switch (c) {
            case 'B':   // Byte        B
            case 'S':   // Short       S
            case 'I':   // Integer     I
            case 'J':   // Long        J
            case 'F':   // Float       F
            case 'D':   // Double      D
            case 'C':   // Char        C
            case 'Z':   // Boolean     Z
            case 'T':   // String      T
            case 'H':   // HANDLE      H
            case 'E':   // ECode       E
            case 'K':   // CoclassID   |     K
            case 'M':   // ComponentID |     M
            case 'U': { // InterfaceID |     U
                buf[0] = c;
                buf[1] = '\0';
                str++;
                break;
            }
            case '[': { // Array       [
                buf[0] = c;
                str++;
                int len = getOneType(str, buf + 1);
                if (len == 0)
                    buf[1] = '\0';
                break;
            }
            case 'R':   // Triple      R
            case 'P':   // Pointer     *
            case '&': { // Reference   &
                buf[0] = c;
                str++;
                buf[1] = *str;
                buf[2] = '\0';
                str++;
                break;
            }
            case 'L': { // Interface   | Lxx/xx;
                        // Enum    Lxx/xx;
                int i = 0;
                str++;
                c = *str;
                while ((c != ';') && (c != '\0') && (i < 256)) {
                    buf[i++] = c;
                    str++;
                    c = *str;
                }
                if (c == ';')
                    str++;

                break;
            }
            default:
                assert(false);
                buf[0] = '\0';
                return 0;
        }
        c = *str;
    }
    else
        return 0;

    return strlen(buf);
}
