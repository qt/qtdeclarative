QT += qml quick

HEADERS += squircle.h
SOURCES += squircle.cpp main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/openglunderqml
qml.files = main.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/openglunderqml
INSTALLS += target qml
