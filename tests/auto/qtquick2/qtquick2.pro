TEMPLATE = subdirs

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    qdeclarativepixmapcache

PRIVATETESTS += \
    qdeclarativeanimations \
    qdeclarativeapplication \
    qdeclarativebehaviors \
    qdeclarativefontloader \
    qdeclarativepath \
    qdeclarativesmoothedanimation \
    qdeclarativespringanimation \
    qdeclarativestyledtext \
    qdeclarativestates \
    qdeclarativesystempalette \
    qdeclarativetimer \
    qdeclarativexmllistmodel

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qdeclarativexmllistmodel

QUICKTESTS =  \
    qquickanchors \
    qquickanimatedimage \
    qquickborderimage \
    qquickcanvas \
    qquickdrag \
    qquickdroparea \
    qquickflickable \
    qquickflipable \
    qquickfocusscope \
    qquickgridview \
    qquickimage \
    qquickitem \
    qquickitem2 \
    qquicklistview \
    qquickloader \
    qquickmousearea \
    qquickmultipointtoucharea \
    qquickpathview \
    qquickpincharea \
    qquickpositioners \
    qquickrepeater \
    qquickshadereffect \
    qquickspriteimage \
    qquicktext \
    qquicktextedit \
    qquicktextinput \
    qquickvisualdatamodel \
    qquickview \
    qquickcanvasitem \


SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
