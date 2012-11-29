TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/stocqt
qml.files = stocqt.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/stocqt
INSTALLS += target qml
