TEMPLATE = app
TARGET = pop-o-rama
QT += quick

CONFIG += use-vkb

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    ../shared/shared.qrc \
    qml.qrc

use-vkb {
    DEFINES += USE_VKB
}
