# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_get_tool_target_name(target_name qmllint)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Syntax Verifier"
    TOOLS_TARGET Qml # special case
    SOURCES
        checkidentifiers.cpp checkidentifiers.h
        findwarnings.cpp findwarnings.h
        codegen.cpp codegen.h
        codegenwarninginterface.cpp codegenwarninginterface.h
        main.cpp
        ../shared/qqmltoolingsettings.h
        ../shared/qqmltoolingsettings.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlCompilerPrivate
)

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
