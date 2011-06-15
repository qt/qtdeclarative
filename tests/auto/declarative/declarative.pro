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
    qdeclarativelayoutitem \
    qdeclarativelistreference \
    qdeclarativemoduleplugin \
    qdeclarativeparticles \
    qdeclarativepixmapcache \
    qdeclarativeqt \
    qdeclarativeview \
    qdeclarativeviewer \
    qdeclarativexmlhttprequest \
    moduleqt47

PRIVATETESTS += \
    qdeclarativeanchors \
    qdeclarativeanimatedimage \
    qdeclarativeanimations \
    qdeclarativeapplication \
    qdeclarativebehaviors \
    qdeclarativebinding \
    qdeclarativeborderimage \
    qdeclarativeconnection \
    qdeclarativedebug \
    qdeclarativedebugclient \
    qdeclarativedebughelper \
    qdeclarativedebugservice \
    qdeclarativeecmascript \
    qdeclarativeflickable \
    qdeclarativeflipable \
    qdeclarativefocusscope \
    qdeclarativefontloader \
    qdeclarativegridview \
    qdeclarativeimage \
    qdeclarativeimageprovider \
    qdeclarativeinstruction \
    qdeclarativeitem \
    qdeclarativelanguage \
    qdeclarativelistmodel \
    qdeclarativelistview \
    qdeclarativeloader \
    qdeclarativemousearea \
    qdeclarativepathview \
    qdeclarativepincharea \
    qdeclarativepositioners \
    qdeclarativeproperty \
    qdeclarativepropertymap \
    qdeclarativerepeater \
    # qdeclarativescriptdebugging \
    qdeclarativesmoothedanimation \
    qdeclarativespringanimation \
    qdeclarativestyledtext \
    qdeclarativesqldatabase \
    qdeclarativestates \
    qdeclarativesystempalette \
    qdeclarativetext \
    qdeclarativetextedit \
    qdeclarativetextinput \
    qdeclarativetimer \
    qdeclarativevaluetypes \
    qdeclarativevisualdatamodel \
    qdeclarativeworkerscript \
    qdeclarativexmllistmodel \
    qpacketprotocol \
    qdeclarativev4 \
    v8

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


SUBDIRS += $$PUBLICTESTS

!symbian: {
    SUBDIRS += $$METATYPETESTS
}

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
    SUBDIRS += $$SGTESTS
}

# Tests which should run in Pulse
PULSE_TESTS = $$SUBDIRS

