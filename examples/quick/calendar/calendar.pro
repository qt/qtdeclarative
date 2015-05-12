TEMPLATE = app
TARGET = calendar
QT += quick sql

SOURCES += \
    main.cpp

OTHER_FILES += \
    main.qml

RESOURCES += \
    calendar.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcalendar2/calendar
INSTALLS += target
