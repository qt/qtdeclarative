TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/touchinteraction
qml.files = flickable multipointtouch pincharea touchinteraction.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/touchinteraction
INSTALLS += target qml
