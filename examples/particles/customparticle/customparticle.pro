TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/customparticle
qml.files = customparticle.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/particles/customparticle
INSTALLS += target qml

