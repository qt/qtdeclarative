TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/xmlhttprequest
qml.files = xmlhttprequest.qml get.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/xmlhttprequest
INSTALLS += target qml
