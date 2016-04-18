TEMPLATE = app
TARGET = gallery
QT += quick

SOURCES += \
    gallery.cpp

OTHER_FILES += \
    gallery.qml

RESOURCES += \
    gallery.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/gallery
INSTALLS += target
