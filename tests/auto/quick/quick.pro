TEMPLATE = subdirs

PUBLICTESTS += \
    geometry \
    qquickpixmapcache

qtConfig(opengl(es1|es2)?) {
    PUBLICTESTS += \
        drawingmodes \
        rendernode
    qtHaveModule(widgets): PUBLICTESTS += nodes

    QUICKTESTS += \
        qquickanimatedsprite \
        qquickframebufferobject \
        qquickspritesequence
        qtHaveModule(opengl): QUICKTESTS += qquickrendercontrol

    QUICKTESTS += qquickshadereffect
# qquickshadereffect test is disabled for now, re-enable when ported to Qt 6
}

!cross_compile: PRIVATETESTS += examples

# This test requires the qtconcurrent module
!qtHaveModule(concurrent): PUBLICTESTS -= qquickpixmapcache

PRIVATETESTS += \
    nokeywords \
    propertyrequirements \
    qquickanimations \
    qquickapplication \
    qquickbehaviors \
    qquickboundaryrule \
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
    qquickcolorgroup \
    qquickpalette \
    qquicktimeline

QUICKTESTS += \
    pointerhandlers \
    qquickaccessible \
    qquickanchors \
    qquickanimatedimage \
    qquickanimators \
    qquickdynamicpropertyanimation \
    qquickborderimage \
    qquickwindow \
    qquickdrag \
    qquickdroparea \
    qquickflickable \
    qquickflipable \
    qquickfocusscope \
    qquickgraphicsinfo \
    qquickgridview \
    qquickimage \
    qquickitem \
    qquickitem2 \
    qquickitemlayer \
    qquicklistview \
    qquicktableview \
    qquickloader \
    qquickmousearea \
    qquickmultipointtoucharea \
    qquickpainteditem \
    qquickshape \
    qquickpathview \
    qquickpincharea \
    qquickpositioners \
    qquickrectangle \
    qquickrepeater \
    qquickshortcut \
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
    scenegraph \
    sharedimage

SUBDIRS += $$PUBLICTESTS

# Following tests are too slow on qemu + software backend
boot2qt: QUICKTESTS -= qquickgridview qquicklistview qquicktableview qquickpositioners

!qtConfig(accessibility):QUICKTESTS -= qquickaccessible

!qtConfig(shortcut):QUICKTESTS -= qquickshortcut

qtConfig(private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
