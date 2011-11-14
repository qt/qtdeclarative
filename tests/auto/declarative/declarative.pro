TEMPLATE = subdirs

METATYPETESTS += \
    qdeclarativemetatype \
    qmetaobjectbuilder

PUBLICTESTS += \
    examples \
    geometry \
    nodes \
    parserstress \
    qdeclarativecomponent \
    qdeclarativecontext \
    qdeclarativeengine \
    qdeclarativeerror \
    qdeclarativefolderlistmodel \
    qdeclarativeincubator \
    qdeclarativeinfo \
    qdeclarativelistreference \
    qdeclarativemoduleplugin \
    qdeclarativepixmapcache \
    qdeclarativeqt \
    qdeclarativetranslation \
    qdeclarativexmlhttprequest \
    qjsvalue \
    qjsvalueiterator \
    qjsengine \
    qmlmin \
    qmlplugindump

PRIVATETESTS += \
    qdeclarativeanimations \
    qdeclarativeapplication \
    qdeclarativebehaviors \
    qdeclarativebinding \
    qdeclarativechangeset \
    qdeclarativeconnection \
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
    qdeclarativestyledtext \
    qdeclarativesqldatabase \
    qdeclarativestates \
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
    qquickspriteimage \
    qquicktext \
    qquicktextedit \
    qquicktextinput \
    qquickvisualdatamodel \
    qquickview \
    qquickcanvasitem \


SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$QUICKTESTS
}
