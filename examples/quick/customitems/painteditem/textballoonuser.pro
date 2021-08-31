TEMPLATE = app

QT += qml quick

TARGET = painteditem
SOURCES += main.cpp
RESOURCES += painteditem.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/painteditem
INSTALLS += target
