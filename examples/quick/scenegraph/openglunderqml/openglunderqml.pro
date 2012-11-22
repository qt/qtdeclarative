QT += qml quick

HEADERS += squircle.h
SOURCES += squircle.cpp main.cpp

OTHER_FILES += main.qml

sources.files = $$SOURCES $$HEADERS $$OTHER_FILES openglunderqml.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/scenegraph/openglunderqml
target.path = $$sources.path
INSTALLS += sources target
