TEMPLATE = subdirs

PUBLICTESTS += \
    geometry \
    rendernode \
    qquickpixmapcache

!contains(QT_CONFIG, no-widgets): PUBLICTESTS += nodes

!cross_compile: PRIVATETESTS += examples

# This test requires the qtconcurrent module
!contains(QT_CONFIG, concurrent):PUBLICTESTS -= qquickpixmapcache

PRIVATETESTS += \
    qquickanimations \
    qquickapplication \
    qquickbehaviors \
    qquickfontloader \
    qquickimageprovider \
    qquickpath \
    qquicksmoothedanimation \
    qquickspringanimation \
    qquickanimationcontroller \
    qquickstyledtext \
    qquickstates \
    qquicksystempalette \
    qquickxmllistmodel

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qquickxmllistmodel

QUICKTESTS =  \
    qquickaccessible \
    qquickanchors \
    qquickanimatedimage \
    qquickanimatedsprite \
    qquickborderimage \
    qquickwindow \
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
    qquickpainteditem \
    qquickpathview \
    qquickpincharea \
    qquickpositioners \
    qquickrectangle \
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
    touchmouse \


SUBDIRS += $$PUBLICTESTS

!contains(QT_CONFIG, accessibility):QUICKTESTS -= qquickaccessible

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
