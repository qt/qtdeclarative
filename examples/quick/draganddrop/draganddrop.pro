TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/draganddrop
qml.files = draganddrop.qml tiles views
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/draganddrop
INSTALLS += target qml

