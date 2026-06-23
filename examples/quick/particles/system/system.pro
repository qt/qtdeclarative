TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    system.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/system
INSTALLS += target
