qt_get_tool_target_name(target_name qmltc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Type Compiler"
    TOOLS_TARGET Qml
    SOURCES
        main.cpp
        qmltccommandlineutils.h qmltccommandlineutils.cpp
        qmltcoutputprimitives.h
        qmltccodewriter.h qmltccodewriter.cpp
        qmltcoutputir.h
        qmltctyperesolver.h
        qmltcvisitor.h qmltcvisitor.cpp
        qmltccompiler.h qmltccompiler.cpp
        qmltccompilerpieces.h
        qmltcpropertyutils.h

        prototype/generatedcodeprimitives.h
        prototype/qml2cppcontext.h
        prototype/visitor.cpp prototype/visitor.h
        prototype/qml2cppdefaultpasses.cpp prototype/qml2cppdefaultpasses.h
        prototype/codegenerator.cpp prototype/codegenerator.h
        prototype/codegeneratorutil.cpp prototype/codegeneratorutil.h
        prototype/codegeneratorwriter.cpp prototype/codegeneratorwriter.h
        prototype/qmlcompiler.h
        prototype/typeresolver.cpp prototype/typeresolver.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::Core
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
)
qt_internal_return_unless_building_tools()
