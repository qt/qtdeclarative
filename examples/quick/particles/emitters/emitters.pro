TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/emitters
qml.files = emitters.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/emitters
INSTALLS += target qml
