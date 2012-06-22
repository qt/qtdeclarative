TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/xmlhttprequest
qml.files = xmlhttprequest.qml get.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/xmlhttprequest
INSTALLS += target qml
