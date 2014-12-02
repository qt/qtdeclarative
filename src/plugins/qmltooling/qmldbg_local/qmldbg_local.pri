QT += core-private

SOURCES += \
    $$PWD/qlocalclientconnection.cpp \
    $$PWD/../shared/qpacketprotocol.cpp

HEADERS += \
    $$PWD/qlocalclientconnection.h \
    $$PWD/../shared/qpacketprotocol.h

INCLUDEPATH += $$PWD \
    $$PWD/../shared

OTHER_FILES += $$PWD/qlocalclientconnection.json
