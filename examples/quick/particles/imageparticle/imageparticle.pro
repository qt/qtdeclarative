TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/imageparticle
qml.files = imageparticle.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/imageparticle
INSTALLS += target qml
