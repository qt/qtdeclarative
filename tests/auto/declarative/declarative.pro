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
    qdeclarativev4

SGTESTS =  \
    qsganimatedimage \
    qsgborderimage \
    qsgcanvas \
    qsgflickable \
    qsgflipable \
    qsgfocusscope \
    qsggridview \
    qsgimage \
    qsgitem \
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

# Tests which should run in Pulse
PULSE_TESTS = $$SUBDIRS
