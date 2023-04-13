# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_get_tool_target_name(target_name qmltc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML type compiler"
    TOOLS_TARGET Qml
    SOURCES
        main.cpp
        qmltccommandlineutils.h qmltccommandlineutils.cpp
        qmltcoutputprimitives.h
        qmltccodewriter.h qmltccodewriter.cpp
        qmltcoutputir.h
        qmltctyperesolver.h qmltctyperesolver.cpp
        qmltcvisitor.h qmltcvisitor.cpp
        qmltccompiler.h qmltccompiler.cpp
        qmltccompilerpieces.h qmltccompilerpieces.cpp
        qmltcpropertyutils.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()
