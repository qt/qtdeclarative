# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## TestTypes module.
##
## The C++ types (BirthdayParty, Person, CppBaseClass, ...) and the Test.qml
## helper used by the qmlcppcodegen benchmark documents in groups/cppcodegen/.
## Ported from qtdeclarative's tests/auto/qml/qmlcppcodegen/data. Built like the
## QmlBench module (plugin-backed static, so the AOT units register; see the
## bundle helper in ../CMakeLists.txt).
#####################################################################

unified_add_qml_module(BenchmarkTestTypes
    URI TestTypes
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/TestTypes"
    SOURCES
        birthdayparty.h birthdayparty.cpp
        person.h person.cpp
        cppbaseclass.h
    QML_FILES
        Test.qml
)

target_link_libraries(BenchmarkTestTypes PUBLIC Qt::Gui Qt::Qml Qt::Quick)
