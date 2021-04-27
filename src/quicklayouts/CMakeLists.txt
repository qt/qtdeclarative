 qt_internal_add_module(QuickLayouts
    SOURCES
        qquickgridlayoutengine.cpp qquickgridlayoutengine_p.h
        qquicklayout.cpp qquicklayout_p.h
        qquicklayoutstyleinfo.cpp qquicklayoutstyleinfo_p.h
        qquicklinearlayout.cpp qquicklinearlayout_p.h
        qquickstacklayout.cpp qquickstacklayout_p.h
        qquicklayoutglobal_p.h
    DEFINES
        QT_BUILD_QUICKLAYOUTS_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::GuiPrivate
        Qt::QuickPrivate
        Qt::Qml
)


set_target_properties(QuickLayouts PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${PROJECT_VERSION}
    QT_QML_MODULE_URI QtQuick.Layouts
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtQuick/Layouts"
)

qt6_qml_type_registration(QuickLayouts)
