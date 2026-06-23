TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    text.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/text
INSTALLS += target
