TEMPLATE = subdirs

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    rendernode \
    qquickpixmapcache

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
    qquickstyledtext \
    qquickstates \
    qquicksystempalette \
    qquicktimer \
    qquickxmllistmodel

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qquickxmllistmodel

# FIXME
# qquickdroparea is disabled because it depends on changes that
# have not been merged from qtbase/master to qtbase/api_changes yet:
    #qquickdroparea \

QUICKTESTS =  \
    qquickaccessible \
    qquickanchors \
    qquickanimatedimage \
    qquickanimatedsprite \
    qquickborderimage \
    qquickcanvas \
    qquickdrag \
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
