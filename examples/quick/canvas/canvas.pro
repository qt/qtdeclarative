TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    canvas.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/canvas
INSTALLS += target
