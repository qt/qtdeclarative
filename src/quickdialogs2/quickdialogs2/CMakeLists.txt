#####################################################################
## QuickDialogs2 Module:
#####################################################################

qt_internal_add_module(QuickDialogs2
    SOURCES
        qquickabstractdialog.cpp
        qquickabstractdialog_p.h
        qquickfiledialog.cpp
        qquickfiledialog_p.h
        qquickfontdialog.cpp
        qquickfontdialog_p.h
        qtquickdialogs2foreign_p.h
        qtquickdialogs2global_p.h
    DEFINES
        QT_BUILD_QUICKDIALOGS2_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickDialogs2UtilsPrivate
        Qt::QuickDialogs2QuickImplPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
)

## Scopes:
#####################################################################

set_target_properties(QuickDialogs2 PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Dialogs
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Dialogs"
)

qt6_qml_type_registration(QuickDialogs2)
