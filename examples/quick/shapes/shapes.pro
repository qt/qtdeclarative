TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    shapes.qrc \
    ../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/shapes
INSTALLS += target
