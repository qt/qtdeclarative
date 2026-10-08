TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    shapes.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickshapes/shapes
INSTALLS += target
