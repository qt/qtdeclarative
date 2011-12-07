target.path     += $$[QT_INSTALL_PLUGINS]/accessible
INSTALLS        += target

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += $$PWD/qdeclarativeaccessible.cpp
HEADERS += $$PWD/qdeclarativeaccessible.h
