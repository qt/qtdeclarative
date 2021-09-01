TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    localstorage.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/localstorage
INSTALLS += target
