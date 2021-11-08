TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    emitters.qrc \
    ../../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/emitters
INSTALLS += target
