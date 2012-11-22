TEMPLATE = lib
CONFIG += qt plugin
QT += qml

DESTDIR +=  ../plugins
OBJECTS_DIR = tmp
MOC_DIR = tmp

TARGET = FileDialog

HEADERS +=     directory.h \
        file.h \
        dialogPlugin.h

SOURCES +=    directory.cpp \
        file.cpp \
        dialogPlugin.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/tutorials/gettingStartedQml/filedialog
sources.files = $$SOURCES $$HEADERS filedialog.pro
sources.path = $$target.path
INSTALLS = sources target
