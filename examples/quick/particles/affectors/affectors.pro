TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    affectors.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/particles/affectors
INSTALLS += target
