TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/keyinteraction
qml.files = keyinteraction.qml focus
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/keyinteraction
INSTALLS += target qml

