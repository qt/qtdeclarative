TEMPLATE = app

QT += gui-private widgets qml quick

SOURCES += main.cpp

RESOURCES += rendercontrol_rhi.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/rendercontrol/rendercontrol_rhi
INSTALLS += target
