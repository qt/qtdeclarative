TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

EXAMPLE_FILES = \
    mousearea-wheel-example.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/mousearea
qml.files = mousearea.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/mousearea
INSTALLS += target qml
