TEMPLATE = app

QT += quick qml quickcontrols2
SOURCES += main.cpp
RESOURCES += \
    imageelements.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/imageelements
INSTALLS += target
