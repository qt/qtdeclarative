TEMPLATE = app
TARGET = theme
QT += quick

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    theme.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/labs/controls/theme
INSTALLS += target
