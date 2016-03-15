TEMPLATE = app
TARGET = gallery
QT += quick labscontrols

SOURCES += \
    gallery.cpp

OTHER_FILES += \
    gallery.qml

RESOURCES += \
    gallery.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtlabscontrols/gallery
INSTALLS += target
