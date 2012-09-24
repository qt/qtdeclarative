TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/localstorage
qml.files = localstorage.qml hello.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/localstorage
INSTALLS += target qml
