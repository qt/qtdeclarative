TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/customparticle
qml.files = customparticle.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/customparticle
INSTALLS += target qml
