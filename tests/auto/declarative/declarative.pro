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
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
#   qdeclarativedebugjs \
    qdeclarativeecmascript \
    qdeclarativeimageprovider \
    qdeclarativeinstruction \
    qdeclarativelanguage \
    qdeclarativelistcompositor \
    qdeclarativelistmodel \
    qdeclarativeproperty \
    qdeclarativepropertymap \
#   qdeclarativescriptdebugging \
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
    qpacketprotocol \
    v4

# This test requires the xmlpatterns module
!contains(QT_CONFIG,xmlpatterns):PRIVATETESTS -= qdeclarativexmllistmodel

SGTESTS =  \
    qsganimatedimage \
    qsgborderimage \
    qsgcanvas \
    qsgdrag \
    qsgdroparea \
    qsgflickable \
    qsgflipable \
    qsgfocusscope \
    qsggridview \
    qsgimage \
    qsgitem \
    qsgitem2 \
    qsglistview \
    qsgloader \
    qsgmousearea \
    qsgpathview \
    qsgpincharea \
    qsgpositioners \
    qsgrepeater \
    qsgtext \
    qsgtextedit \
    qsgtextinput \
    qsgvisualdatamodel \
    qsgcanvasitem \


SUBDIRS += $$PUBLICTESTS

SUBDIRS += $$METATYPETESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$SGTESTS
}
