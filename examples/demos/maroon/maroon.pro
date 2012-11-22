TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/maroon
qml.files = maroon.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos/maroon
sources.files = $$SOURCES maroon.pro
sources.path = $$qml.path
INSTALLS += sources target qml
