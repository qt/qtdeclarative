TEMPLATE = app

QT += qml quick

SOURCES += main.cpp

RESOURCES += \
    responsivelayouts.qrc
EXAMPLE_FILES = \
    responsivelayouts.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/responsivelayouts
INSTALLS += target

