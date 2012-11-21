TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml

!isEmpty(QT.gui.name) {
    SUBDIRS += \
        quick \
        qmltest \
        particles
}

SUBDIRS += \
    plugins \
    imports \
    qmldevtools
