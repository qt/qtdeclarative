TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/localstorage/localstorage
qml.files = localstorage.qml hello.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/localstorage/localstorage
sources.files = $$SOURCES localstorage.pro
sources.path = $$qml.path
INSTALLS += sources target qml
