target.path     += $$[QT_INSTALL_PLUGINS]/accessible
INSTALLS        += target

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
SOURCES += $$PWD/qqmlaccessible.cpp
HEADERS += $$PWD/qqmlaccessible.h
