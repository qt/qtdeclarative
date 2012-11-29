TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/imageelements
qml.files = *.qml content
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/imageelements
INSTALLS += target qml
