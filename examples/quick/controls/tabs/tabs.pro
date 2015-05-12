TEMPLATE = app
TARGET = tabs
QT += quick

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    tabs.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/tabs
INSTALLS += target
