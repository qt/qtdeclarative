TEMPLATE = app

QT += quick qml
QT += quickcontrols2
SOURCES += main.cpp
RESOURCES += \
            qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/multieffect/testbed
INSTALLS += target
