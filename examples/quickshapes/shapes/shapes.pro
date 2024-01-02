TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    shapes.qrc \
    ../../quick/shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickshapes/shapes
INSTALLS += target
