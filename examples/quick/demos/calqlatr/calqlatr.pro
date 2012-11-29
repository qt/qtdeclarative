TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/calqlatr
qml.files = calqlatr.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/calqlatr
INSTALLS += target qml
