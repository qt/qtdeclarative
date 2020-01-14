TEMPLATE = app
TARGET = qquickviewcomparison

CONFIG += qmltypes
QML_IMPORT_NAME = fbitem
QML_IMPORT_MAJOR_VERSION = 1

QT += quick widgets quickwidgets

SOURCES += main.cpp \
           mainwindow.cpp \
           logo.cpp \
           fbitem.cpp

HEADERS += mainwindow.h \
           logo.h \
           fbitem.h

RESOURCES += qquickviewcomparison.qrc

OTHER_FILES += test.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/quickwidgets/qquickviewcomparison
INSTALLS += target
