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
    nokeywords \
    qquickanimations \
    qquickapplication \
    qquickbehaviors \
    qquickfontloader \
    qquickfontloader_static \
    qquickfontmetrics \
    qquickimageprovider \
    qquicklayouts \
    qquickpath \
    qquicksmoothedanimation \
    qquickspringanimation \
    qquickanimationcontroller \
    qquickstyledtext \
    qquickstates \
    qquicksystempalette \
    qquicktimeline \
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
    qquickframebufferobject \
    qquickgridview \
    qquickimage \
    qquickitem \
    qquickitem2 \
    qquickitemlayer \
    qquicklistview \
    qquickloader \
    qquickmousearea \
    qquickmultipointtoucharea \
    qquickopenglinfo \
    qquickpainteditem \
    qquickpathview \
    qquickpincharea \
    qquickpositioners \
    qquickrectangle \
    qquickrepeater \
    qquickshadereffect \
    qquickshortcut \
    qquickspritesequence \
    qquicktext \
    qquicktextdocument \
    qquicktextedit \
    qquicktextinput \
    qquickvisualdatamodel \
    qquickview \
    qquickcanvasitem \
    qquickdesignersupport \
    qquickscreen \
    touchmouse \
    scenegraph

SUBDIRS += $$PUBLICTESTS

!contains(QT_CONFIG, accessibility):QUICKTESTS -= qquickaccessible

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
