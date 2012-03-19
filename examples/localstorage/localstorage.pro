TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/localstorage
qml.files = localstorage.qml hello.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/localstorage
INSTALLS += target qml