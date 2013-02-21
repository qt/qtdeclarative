TEMPLATE = subdirs

METATYPETESTS += \
    qqmlmetatype

PUBLICTESTS += \
    parserstress \
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
    qqmlnotifier \
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
    qqmlpropertycache \
    qqmlpropertymap \
    qqmlsqldatabase \
    qqmlvaluetypes \
    qqmlvaluetypeproviders \
    qqmlbinding \
    qquickchangeset \
    qqmlconnections \
    qquicklistcompositor \
    qquicklistmodel \
    qquicklistmodelworkerscript \
    qquickworkerscript \
    qqmlbundle \
    qrcqml \
    v4 \
    qqmltimer

qtHaveModule(widgets) {
    PUBLICTESTS += \
        qjsengine \
        qjsvalue
}

SUBDIRS += $$PUBLICTESTS
SUBDIRS += $$METATYPETESTS
SUBDIRS += debugger

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
