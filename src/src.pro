TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml

qtHaveModule(gui) {
    SUBDIRS += \
        quick \
        qmltest \
        particles
}

SUBDIRS += \
    plugins \
    imports \
    qmldevtools
