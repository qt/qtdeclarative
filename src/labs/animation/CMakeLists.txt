qt_internal_add_qml_module(LabsAnimation
    URI "Qt.labs.animation"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET labsanimationplugin
    CLASS_NAME QtLabsAnimationPlugin
    SOURCES
        qquickboundaryrule.cpp qquickboundaryrule_p.h
        qqmlanimationglobal_p.h
    DEFINES
        QT_BUILD_LABSANIMATION_LIB
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
        Qt::QuickPrivate
)
