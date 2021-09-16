qt_get_tool_target_name(target_name qmltc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Type Compiler"
    TOOLS_TARGET Qml
    SOURCES
        main.cpp
        qmltccommandlineutils.h qmltccommandlineutils.cpp
        qmltctyperesolver.h
        qmltcvisitor.h
        qmltccompiler.h qmltccompiler.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()
