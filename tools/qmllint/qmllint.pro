option(host_build)

QT = core-private qmldevtools-private

include(../shared/shared.pri)

SOURCES += \
    $$METATYPEREADER_SOURCES \
    checkidentifiers.cpp \
    main.cpp \
    findwarnings.cpp \
    importedmembersvisitor.cpp \
    qcoloroutput.cpp

QMAKE_TARGET_DESCRIPTION = QML Syntax Verifier

load(qt_tool)

HEADERS += \
    $$METATYPEREADER_HEADERS \
    checkidentifiers.h \
    findwarnings.h \
    importedmembersvisitor.h \
    qcoloroutput.h
