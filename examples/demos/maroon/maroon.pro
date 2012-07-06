TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/maroon
qml.files = maroon.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/demos/maroon
INSTALLS += target qml
