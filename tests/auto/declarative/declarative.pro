TEMPLATE = subdirs
!symbian: {
SUBDIRS += \
           qdeclarativemetatype \
           qmetaobjectbuilder
}

SUBDIRS += \
           examples \
           geometry \
           node \
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

contains(QT_CONFIG, private_tests) {
    SUBDIRS += \
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
#           qdeclarativescriptdebugging \
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
           v8

}

# Tests which should run in Pulse
PULSE_TESTS = $$SUBDIRS
