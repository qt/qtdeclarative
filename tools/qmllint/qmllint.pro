option(host_build)

QT = core-private qmldevtools-private qmlcompiler-private

SOURCES += \
    checkidentifiers.cpp \
    main.cpp \
    findwarnings.cpp \
    qcoloroutput.cpp

QMAKE_TARGET_DESCRIPTION = QML Syntax Verifier

load(qt_tool)

HEADERS += \
    checkidentifiers.h \
    findwarnings.h \
    qcoloroutput.h
