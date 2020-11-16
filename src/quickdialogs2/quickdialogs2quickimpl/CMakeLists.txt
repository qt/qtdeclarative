#####################################################################
## QuickDialogs2QuickImpl Module:
#####################################################################

qt_internal_add_module(QuickDialogs2QuickImpl
    SOURCES
        qquickdialogimplfactory.cpp
        qquickdialogimplfactory_p.h
        qquickfiledialogdelegate.cpp
        qquickfiledialogdelegate_p.h
        qquickfiledialogimpl.cpp
        qquickfiledialogimpl_p.h
        qquickfiledialogimpl_p_p.h
        qquickfolderbreadcrumbbar.cpp
        qquickfolderbreadcrumbbar_p.h
        qquickfolderbreadcrumbbar_p_p.h
        qquickplatformfiledialog.cpp
        qquickplatformfiledialog_p.h
        qtquickdialogs2quickimplforeign_p.h
        qtquickdialogs2quickimplglobal_p.h
    DEFINES
        QT_BUILD_QUICKDIALOGS2QUICKIMPL_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickDialogs2UtilsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
)

# Resources:
set(qtquickdialogs2quickimpl_resource_files
    "images/crumb-separator-icon-round.png"
    "images/crumb-separator-icon-round@2x.png"
    "images/crumb-separator-icon-round@3x.png"
    "images/crumb-separator-icon-round@4x.png"
    "images/crumb-separator-icon-square.png"
    "images/crumb-separator-icon-square@2x.png"
    "images/crumb-separator-icon-square@3x.png"
    "images/crumb-separator-icon-square@4x.png"
    "images/file-icon-round.png"
    "images/file-icon-round@2x.png"
    "images/file-icon-round@3x.png"
    "images/file-icon-round@4x.png"
    "images/file-icon-square.png"
    "images/file-icon-square@2x.png"
    "images/file-icon-square@3x.png"
    "images/file-icon-square@4x.png"
    "images/folder-icon-round.png"
    "images/folder-icon-round@2x.png"
    "images/folder-icon-round@3x.png"
    "images/folder-icon-round@4x.png"
    "images/folder-icon-square.png"
    "images/folder-icon-square@2x.png"
    "images/folder-icon-square@3x.png"
    "images/folder-icon-square@4x.png"
    "images/imagine/filedialogdelegate-background@2x.9.png"
    "images/imagine/filedialogdelegate-background@3x.9.png"
    "images/imagine/filedialogdelegate-background@4x.9.png"
    "images/imagine/filedialogdelegate-background.9.png"
    "images/imagine/filedialogdelegate-background-disabled@2x.9.png"
    "images/imagine/filedialogdelegate-background-disabled@3x.9.png"
    "images/imagine/filedialogdelegate-background-disabled@4x.9.png"
    "images/imagine/filedialogdelegate-background-disabled.9.png"
    "images/imagine/filedialogdelegate-background-pressed@2x.9.png"
    "images/imagine/filedialogdelegate-background-pressed@3x.9.png"
    "images/imagine/filedialogdelegate-background-pressed@4x.9.png"
    "images/imagine/filedialogdelegate-background-pressed.9.png"
    "images/imagine/filedialogdelegate-background-focused@2x.9.png"
    "images/imagine/filedialogdelegate-background-focused@3x.9.png"
    "images/imagine/filedialogdelegate-background-focused@4x.9.png"
    "images/imagine/filedialogdelegate-background-focused.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@2x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@3x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted@4x.9.png"
    "images/imagine/filedialogdelegate-background-highlighted.9.png"
    "images/up-icon-round.png"
    "images/up-icon-round@2x.png"
    "images/up-icon-round@3x.png"
    "images/up-icon-round@4x.png"
    "images/up-icon-square.png"
    "images/up-icon-square@2x.png"
    "images/up-icon-square@3x.png"
    "images/up-icon-square@4x.png"
    "images/up-icon-thick-square.png"
    "images/up-icon-thick-square@2x.png"
    "images/up-icon-thick-square@3x.png"
    "images/up-icon-thick-square@4x.png"
    "qml/FileDialog.qml"
    "qml/FileDialogDelegate.qml"
    "qml/FileDialogDelegateLabel.qml"
    "qml/FolderBreadcrumbBar.qml"
    "qml/+Fusion/FileDialog.qml"
    "qml/+Fusion/FileDialogDelegate.qml"
    "qml/+Fusion/FolderBreadcrumbBar.qml"
    "qml/+Imagine/FileDialog.qml"
    "qml/+Imagine/FileDialogDelegate.qml"
    "qml/+Imagine/FolderBreadcrumbBar.qml"
    "qml/+Material/FileDialog.qml"
    "qml/+Material/FileDialogDelegate.qml"
    "qml/+Material/FolderBreadcrumbBar.qml"
    "qml/+Universal/FileDialog.qml"
    "qml/+Universal/FileDialogDelegate.qml"
    "qml/+Universal/FolderBreadcrumbBar.qml"
)

qt_internal_add_resource(QuickDialogs2QuickImpl "QuickDialogs2QuickImpl"
    PREFIX
        "/qt-project.org/imports/QtQuick/Dialogs/quickimpl"
    FILES
        ${qtquickdialogs2quickimpl_resource_files}
)

## Scopes:
#####################################################################

set_target_properties(QuickDialogs2QuickImpl PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Dialogs.quickimpl
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Dialogs/quickimpl"
)

qt6_qml_type_registration(QuickDialogs2QuickImpl)
