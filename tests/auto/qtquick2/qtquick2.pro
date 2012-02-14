TEMPLATE = subdirs

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    rendernode \
    qdeclarativepixmapcache

# This test requires the qtconcurrent module
!contains(QT_CONFIG, concurrent):PUBLICTESTS -= qdeclarativepixmapcache

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
    qquickaccessible \
    qquickanchors \
    qquickanimatedimage \
    qquickanimatedsprite \
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
    qquickitemlayer \
    qquicklistview \
    qquickloader \
    qquickmousearea \
    qquickmultipointtoucharea \
    qquickpathview \
    qquickpincharea \
    qquickpositioners \
    qquickrepeater \
    qquickshadereffect \
    qquickspritesequence \
    qquicktext \
    qquicktextedit \
    qquicktextinput \
    qquickvisualdatamodel \
    qquickview \
    qquickcanvasitem \
    qquickscreen \


SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
