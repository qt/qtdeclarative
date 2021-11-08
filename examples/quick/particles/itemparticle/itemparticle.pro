TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    itemparticle.qrc \
    ../../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/itemparticle
INSTALLS += target
