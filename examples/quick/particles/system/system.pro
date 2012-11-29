TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/system
qml.files = system.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/system
INSTALLS += target qml
