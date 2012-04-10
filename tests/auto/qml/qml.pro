TEMPLATE = subdirs

METATYPETESTS += \
    qqmlmetatype

PUBLICTESTS += \
    parserstress \
    qjsengine \
    qjsvalue \
    qjsvalueiterator \
    qjsonbinding \
    qmlmin \
    qmlplugindump \
    qqmlcomponent \
    qqmlconsole \
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
    qqmlcontext \
    qqmlexpression \
    qqmlglobal \
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
    qquicklistmodelworkerscript \
    qquickworkerscript \
    qqmlbundle \
    v4

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
