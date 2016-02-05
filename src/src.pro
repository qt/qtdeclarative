TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml

qtHaveModule(gui) {
    SUBDIRS += \
        quick

    contains(QT_CONFIG, opengl(es1|es2)?) {
        SUBDIRS += \
            qmltest \
            particles

        qtHaveModule(widgets): SUBDIRS += quickwidgets
    }
}

SUBDIRS += \
    plugins \
    imports \
    qmldevtools

!contains(QT_CONFIG, no-qml-debug): SUBDIRS += qmldebug

qmldevtools.CONFIG = host_build
