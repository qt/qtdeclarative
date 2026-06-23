TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    positioners.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/positioners
INSTALLS += target
