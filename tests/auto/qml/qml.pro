TEMPLATE = subdirs

METATYPETESTS += \
    qqmlmetatype

PUBLICTESTS += \
    parserstress \
    qjsengine \
    qjsvalue \
    qjsvalueiterator \
    qmlmin \
    qmlplugindump \
    qqmlcomponent \
    qqmlconsole \
    qqmlcontext \
    qqmlengine \
    qqmlerror \
    qqmlincubator \
    qqmlinfo \
    qqmllistreference \
    qqmllocale \
    qqmlmetaobject \
    qqmlmoduleplugin \
    qqmlqt \
    qqmltranslation \
    qqmlxmlhttprequest \
    qqmlparser \
    qquickfolderlistmodel

PRIVATETESTS += \
    animation \
    qqmlcpputils \
    qqmlecmascript \
    qqmlexpression \
    qqmlinstruction \
    qqmllanguage \
    qqmlproperty \
    qqmlpropertymap \
    qqmlsqldatabase \
    qqmlvaluetypes \
    qquickbinding \
    qquickchangeset \
    qquickconnection \
    qquicklistcompositor \
    qquicklistmodel \
    qquickworkerscript \
    v4

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
