TEMPLATE = app
TARGET = drawer
QT += quick

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    drawer.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/drawer
INSTALLS += target
