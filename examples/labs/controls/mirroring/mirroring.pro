TEMPLATE = app
TARGET = mirroring
QT += quick

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    mirroring.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/labs/controls/mirroring
INSTALLS += target
