TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/localstorage
qml.files = localstorage.qml hello.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/localstorage
INSTALLS += target qml
