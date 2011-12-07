TEMPLATE = subdirs

METATYPETESTS += \
    qdeclarativemetatype

PUBLICTESTS += \
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
    qdeclarativeqt \
    qdeclarativetranslation \
    qdeclarativexmlhttprequest \
    qdeclarativeparser \
    qjsengine \
    qjsvalue \
    qjsvalueiterator \
    qmlmin \
    qmlplugindump

PRIVATETESTS += \
    qdeclarativeaccessibility \
    qdeclarativebinding \
    qdeclarativechangeset \
    qdeclarativeconnection \
    qdeclarativecpputils \
    qdeclarativeecmascript \
    qdeclarativeexpression \
    qdeclarativeimageprovider \
    qdeclarativeinstruction \
    qdeclarativelanguage \
    qdeclarativelistcompositor \
    qdeclarativelistmodel \
    qdeclarativeproperty \
    qdeclarativepropertymap \
    qdeclarativesqldatabase \
    qdeclarativevaluetypes \
    qdeclarativeworkerscript \
    v4

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
