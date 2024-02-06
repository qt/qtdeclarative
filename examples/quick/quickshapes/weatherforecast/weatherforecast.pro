TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    weatherforecast.qrc \
    ../../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickshapes/weatherforecast
INSTALLS += target
