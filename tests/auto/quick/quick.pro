TEMPLATE = subdirs

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    rendernode \
    qquickpixmapcache

# This test requires the qtconcurrent module
!contains(QT_CONFIG, concurrent):PUBLICTESTS -= qqmlpixmapcache

PRIVATETESTS += \
    qquickanimations \
    qquickapplication \
    qquickbehaviors \
    qquickfontloader \
    qquickpath \
    qquicksmoothedanimation \
    qquickspringanimation \
    qquickstyledtext \
    qquickstates \
    qquicksystempalette \
    qquicktimer \
    qquickxmllistmodel

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qqmlxmllistmodel

QUICKTESTS =  \
    qquickaccessible \
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
    qquickspriteimage \
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
