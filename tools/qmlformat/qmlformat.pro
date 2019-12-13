option(host_build)

QT = core qmldevtools-private

SOURCES += main.cpp \
    commentastvisitor.cpp \
    dumpastvisitor.cpp \
    restructureastvisitor.cpp

QMAKE_TARGET_DESCRIPTION = QML Formatter

HEADERS += \
    commentastvisitor.h \
    dumpastvisitor.h \
    restructureastvisitor.h

load(qt_tool)
