# Generated from nokeywords.pro.

#####################################################################
## tst_nokeywords Test:
#####################################################################

qt_add_test(tst_nokeywords
    SOURCES
        tst_nokeywords.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
)

## Scopes:
#####################################################################

qt_extend_target(tst_nokeywords CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGLPrivate
)
