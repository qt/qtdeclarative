TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    draganddrop.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/draganddrop
INSTALLS += target
