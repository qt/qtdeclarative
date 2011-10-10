load(qttest_p4)
TEMPLATE = app
TARGET = qmltime
QT += declarative widgets
macx:CONFIG -= app_bundle

SOURCES += qmltime.cpp 

