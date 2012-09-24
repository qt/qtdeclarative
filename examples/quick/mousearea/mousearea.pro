TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/mousearea
qml.files = mousearea.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/mousearea
INSTALLS += target qml

