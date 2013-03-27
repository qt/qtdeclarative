TEMPLATE = subdirs

PUBLICTESTS += \
    geometry \
    rendernode \
    qquickpixmapcache

qtHaveModule(widgets): PUBLICTESTS += nodes

!cross_compile: PRIVATETESTS += examples

# This test requires the qtconcurrent module
!qtHaveModule(concurrent): PUBLICTESTS -= qquickpixmapcache

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
!qtHaveModule(xmlpatterns): PRIVATETESTS -= qquickxmllistmodel

QUICKTESTS =  \
    qquickaccessible \
    qquickanchors \
    qquickanimatedimage \
    qquickanimatedsprite \
    qquickdynamicpropertyanimation \
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
    qquicktextdocument \
    qquicktextedit \
    qquicktextinput \
    qquickvisualdatamodel \
    qquickview \
    qquickcanvasitem \
    qquickscreen \
    touchmouse \
    dialogs \


SUBDIRS += $$PUBLICTESTS

!contains(QT_CONFIG, accessibility):QUICKTESTS -= qquickaccessible

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
