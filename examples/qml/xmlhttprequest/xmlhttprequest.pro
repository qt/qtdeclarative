TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += \
    xmlhttprequest.qrc

EXAMPLE_FILES = \
    data.xml

target.path = $$[QT_INSTALL_EXAMPLES]/qml/xmlhttprequest
INSTALLS += target

DISTFILES += \
    methods.js
