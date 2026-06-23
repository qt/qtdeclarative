TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    qml.qrc \

target.path = $$[QT_INSTALL_EXAMPLES]/quick/pointerhandlers
INSTALLS += target
