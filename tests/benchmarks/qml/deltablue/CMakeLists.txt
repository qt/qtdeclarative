# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_benchmark(tst_deltablue
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Qml
        Qt::Test
)


set_source_files_properties(
        Main.qml
        Strength.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE True
)

qt_add_qml_module(tst_deltablue
    URI DeltaBlue
    VERSION 1.0
    QML_FILES
        BaseConstraint.qml
        ChainTest.qml
        Constraint.qml
        Main.qml
        Main2.qml
        Plan.qml
        Planner.qml
        ProjectionTest.qml
        Satisfaction.qml
        Strength.qml
        Variable.qml
        deltablue.js
)

