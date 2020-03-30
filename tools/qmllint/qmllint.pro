option(host_build)

QT = core-private qmldevtools-private

include(../shared/shared.pri)

SOURCES += \
    $$METATYPEREADER_SOURCES \
    checkidentifiers.cpp \
    main.cpp \
    findunqualified.cpp \
    importedmembersvisitor.cpp \
    qcoloroutput.cpp

QMAKE_TARGET_DESCRIPTION = QML Syntax Verifier

load(qt_tool)

HEADERS += \
    $$METATYPEREADER_HEADERS \
    checkidentifiers.h \
    findunqualified.h \
    importedmembersvisitor.h \
    qcoloroutput.h
