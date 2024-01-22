TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    weatherforecast.qrc \
    ../../quick/shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickshapes/weatherforecast
INSTALLS += target
