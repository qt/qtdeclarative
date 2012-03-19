TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/xmlhttprequest
qml.files = xmlhttprequest.qml get.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/xmlhttprequest
INSTALLS += target qml