QT = network core qmldebug-private
CONFIG += no_import_scan

SOURCES += \
    main.cpp \
    qmlpreviewapplication.cpp

HEADERS += \
    qmlpreviewapplication.h

QMAKE_TARGET_DESCRIPTION = QML Preview

load(qt_tool)
