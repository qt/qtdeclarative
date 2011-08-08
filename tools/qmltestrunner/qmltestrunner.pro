TEMPLATE = app
TARGET = qmltestrunner
CONFIG += warn_on
SOURCES += main.cpp


QT += declarative qmltest

macx: CONFIG -= app_bundle

#DEFINES += QUICK_TEST_SOURCE_DIR=\"\\\"$$OUT_PWD\\\"\"

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target