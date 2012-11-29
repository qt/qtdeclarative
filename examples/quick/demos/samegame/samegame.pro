TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/samegame
qml.files = samegame.qml content settings.js
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/samegame
INSTALLS += target qml
