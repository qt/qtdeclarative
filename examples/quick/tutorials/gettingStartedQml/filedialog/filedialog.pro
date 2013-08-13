TEMPLATE = lib
CONFIG += plugin
QT += qml

DESTDIR +=  ../imports/FileDialog
OBJECTS_DIR = tmp
MOC_DIR = tmp

TARGET = filedialogplugin

HEADERS += \
        directory.h \
        file.h \
        dialogPlugin.h

SOURCES += \
        directory.cpp \
        file.cpp \
        dialogPlugin.cpp

OTHER_FILES += qmldir

copyfile = $$PWD/qmldir
copydest = $$DESTDIR

# On Windows, use backslashes as directory separators
win32: {
    copyfile ~= s,/,\\,g
    copydest ~= s,/,\\,g
}

# Copy the qmldir file to the same folder as the plugin binary
QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$copyfile) $$quote($$copydest) $$escape_expand(\\n\\t)
