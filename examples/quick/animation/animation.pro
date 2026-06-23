TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp

RESOURCES += \
    animation.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/animation
INSTALLS += target
