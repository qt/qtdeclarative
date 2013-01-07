TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

RESOURCES += calqlatr.qrc \
    ../../shared/shared.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/calqlatr
INSTALLS += target
