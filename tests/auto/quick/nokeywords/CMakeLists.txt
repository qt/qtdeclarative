# Generated from nokeywords.pro.

#####################################################################
## tst_nokeywords Test:
#####################################################################

qt_add_test(tst_nokeywords
    SOURCES
        tst_nokeywords.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

## Scopes:
#####################################################################

qt_extend_target(tst_nokeywords CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGLPrivate
)
