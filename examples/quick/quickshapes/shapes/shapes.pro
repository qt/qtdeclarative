TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    shapes.qrc \
    ../../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickshapes/shapes
INSTALLS += target
