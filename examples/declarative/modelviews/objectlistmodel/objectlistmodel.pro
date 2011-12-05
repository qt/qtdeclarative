TEMPLATE = app
TARGET = objectlistmodel
DEPENDPATH += .
INCLUDEPATH += .
QT += declarative quick

# Input
SOURCES += main.cpp \
           dataobject.cpp
HEADERS += dataobject.h
RESOURCES += objectlistmodel.qrc


