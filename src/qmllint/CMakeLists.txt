qt_internal_add_module(QmlLintPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        codegen_p.h codegen.cpp
        codegenwarninginterface_p.h codegenwarninginterface.cpp
        findwarnings_p.h findwarnings.cpp
        qqmllinter_p.h qqmllinter.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::QmlPrivate
        Qt::QmlCompilerPrivate
)
