TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    imageparticle.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/imageparticle
INSTALLS += target
