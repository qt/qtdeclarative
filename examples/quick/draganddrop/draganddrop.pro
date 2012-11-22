TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/draganddrop
qml.files = draganddrop.qml tiles views
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/draganddrop
sources.files = $$SOURCES draganddrop.pro
sources.path = $$qml.path
INSTALLS += sources target qml
