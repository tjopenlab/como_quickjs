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

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <comoapi.h>
#include "como_bridge.h"

using namespace pybind11::literals;
namespace py = pybind11;

using namespace como;

static py::module_ *this_pymodule = nullptr;
static MetaComponent *metaComponent = nullptr;
std::map<std::string, py::class_<ComoPyClassStub>> g_como_classes;

PYBIND11_MODULE(como_pybind, m) {
    this_pymodule = &m;

    // load COMO component meta data
    py::class_<MetaComponent> pymc = py::class_<MetaComponent>(m, "como");
    pymc.def(py::init([m](const std::string &str_) {
            metaComponent = new MetaComponent(str_);

            for(int i = 0;  i < metaComponent->como_classes.size();  i++) {
                MetaCoclass *metaCoclass = metaComponent->como_classes[i];
                std::string className = metaCoclass->GetName();
                std::string classNs = metaCoclass->GetNamespace();

                Logger::V("como_pybind", "load class, className: %s\n", className.c_str());
                py::class_<ComoPyClassStub> clz_ = py::class_<ComoPyClassStub>(m, className.c_str(),
                                                                               py::module_local());
                g_como_classes.insert(std::pair<std::string, py::class_<ComoPyClassStub>>(
                                                                    classNs + "." + className, clz_));

                switch (i) {

#define LAMBDA_FOR_CLASS_INIT(_NO_)                                                                     \
                    case _NO_:                                                                          \
                        clz_.def(py::init([](py::args args, py::kwargs kwargs) {                        \
                            MetaCoclass *metacc = metaComponent->como_classes[_NO_];                    \
                            if (args.size() == 0) {                                                     \
                                AutoPtr<IInterface> thisObject = metacc->CreateObject();                \
                                if (thisObject == nullptr) {                                            \
                                    std::string className = std::string(metacc->GetName());             \
                                    throw std::runtime_error("initialize COMO class: " + className);    \
                                }                                                                       \
                                ComoPyClassStub* stub = new ComoPyClassStub(metacc, thisObject);        \
                                return stub;                                                            \
                            } else {                                                                    \
                                ComoPyClassStub* stub = new ComoPyClassStub(metacc);                    \
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

                clz_.def("getAllConstants", &ComoPyClassStub::GetAllConstants);

                Array<IMetaMethod*> methods;
                char buf[MAX_METHOD_NAME_LENGTH];
                for (int j = 0;  j < metaCoclass->methodNumber; j++) {
                    metaCoclass->GetMethodName(j, buf);
                    Logger::V("como_pybind", "load method, methodName: %s\n", buf);
                    switch (j) {

#define LAMBDA_FOR_METHOD(_NO_)                                         \
                        case _NO_:                                      \
                            clz_.def(buf, &ComoPyClassStub::m##_NO_);   \
                            break;

#include "LAMBDA_FOR_METHOD.inc"
#undef LAMBDA_FOR_METHOD
                    }
                }
            }
            return metaComponent;
        }))
        .def("getName", &MetaComponent::GetName)
        .def(
            "getAllConstants",
            [](MetaComponent& m) {
                return m.GetAllConstants();
            },
            py::return_value_policy::take_ownership)

        .def("__repr__",
            [](const MetaComponent &a) {
                return "<MetaComponent componentPath '" + a.componentPath + "'>";
            }
    );
}
