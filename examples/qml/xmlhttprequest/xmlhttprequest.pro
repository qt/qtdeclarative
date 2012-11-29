TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

EXAMPLE_FILES = \
    data.xml

target.path = $$[QT_INSTALL_EXAMPLES]/qml/xmlhttprequest
qml.files = xmlhttprequest.qml get.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/qml/xmlhttprequest
INSTALLS += target qml
