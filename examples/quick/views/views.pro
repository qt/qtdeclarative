TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    views.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/views
INSTALLS += target
