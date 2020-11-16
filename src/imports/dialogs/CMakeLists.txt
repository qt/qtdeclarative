qt_internal_add_qml_module(qtquickdialogsplugin
    URI "QtQuick.Dialogs"
    VERSION "${CMAKE_PROJECT_VERSION}"
    CLASSNAME QtQuickDialogsPlugin
    SKIP_TYPE_REGISTRATION
    SOURCES
        qtquickdialogsplugin.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        quick
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickDialogs2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

qt_internal_add_docs(qtquickdialogsplugin
    doc/qtquickdialogs.qdocconf
)
