TARGET = threadedsonglist
QT += qml quick

HEADERS = datastorage.h \
          mediaelement.h \
          queueworker.h \
          remotemedia.h \
          songdatagenerator.h \
          threadedlistmodel.h
SOURCES = main.cpp \
          datastorage.cpp \
          mediaelement.cpp \
          queueworker.cpp \
          remotemedia.cpp \
          songdatagenerator.cpp \
          threadedlistmodel.cpp
RESOURCES += threadedsonglist.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/models/threadedsonglist
INSTALLS += target
