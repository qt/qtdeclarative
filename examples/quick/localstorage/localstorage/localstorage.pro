TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/localstorage/localstorage
qml.files = localstorage.qml hello.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/localstorage/localstorage
INSTALLS += target qml
