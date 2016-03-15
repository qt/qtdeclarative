TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml \
    quick \
    qmltest

qtHaveModule(gui):contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += particles
    qtHaveModule(widgets): SUBDIRS += quickwidgets
}

SUBDIRS += \
    plugins \
    doc \
    imports \
    qmldevtools

!contains(QT_CONFIG, no-qml-debug): SUBDIRS += qmldebug

qmldevtools.CONFIG = host_build
