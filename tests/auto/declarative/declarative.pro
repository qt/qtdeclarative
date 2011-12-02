TEMPLATE = subdirs

METATYPETESTS += \
    qdeclarativemetatype

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    parserstress \
    qdeclarativecomponent \
    qdeclarativeconsole \
    qdeclarativecontext \
    qdeclarativeengine \
    qdeclarativeerror \
    qdeclarativefolderlistmodel \
    qdeclarativeincubator \
    qdeclarativeinfo \
    qdeclarativelistreference \
    qdeclarativelocale \
    qdeclarativemoduleplugin \
    qdeclarativepixmapcache \
    qdeclarativeqt \
    qdeclarativetranslation \
    qdeclarativexmlhttprequest \
    qjsengine \
    qjsvalue \
    qjsvalueiterator \
    qmlmin \
    qmlplugindump

PRIVATETESTS += \
    qdeclarativeanimations \
    qdeclarativeapplication \
    qdeclarativebehaviors \
    qdeclarativebinding \
    qdeclarativechangeset \
    qdeclarativeconnection \
    qdeclarativecpputils \
    qdeclarativeecmascript \
    qdeclarativeexpression \
    qdeclarativefontloader \
    qdeclarativeimageprovider \
    qdeclarativeinstruction \
    qdeclarativelanguage \
    qdeclarativelistcompositor \
    qdeclarativelistmodel \
    qdeclarativepath \
    qdeclarativeproperty \
    qdeclarativepropertymap \
    qdeclarativesmoothedanimation \
    qdeclarativespringanimation \
    qdeclarativesqldatabase \
    qdeclarativestates \
    qdeclarativestyledtext \
    qdeclarativesystempalette \
    qdeclarativetimer \
    qdeclarativevaluetypes \
    qdeclarativeworkerscript \
    qdeclarativexmllistmodel \
    v4

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qdeclarativexmllistmodel

QUICKTESTS =  \
    qquickanchors \
    qquickanimatedimage \
    qquickborderimage \
    qquickcanvas \
    qquickcanvasitem \
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
    qquickview \
    qquickvisualdatamodel \


SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
