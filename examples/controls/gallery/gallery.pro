TEMPLATE = app
TARGET = gallery
QT += quick quickcontrols2

SOURCES += \
    gallery.cpp

OTHER_FILES += \
    gallery.qml \
    pages/*.qml

RESOURCES += \
    gallery.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtlabscontrols/gallery
INSTALLS += target
