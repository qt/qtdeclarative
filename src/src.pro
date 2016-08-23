TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml \
    quick \
    qmltest

qtHaveModule(gui):qtConfig(opengl(es1|es2)?): \
    SUBDIRS += particles

qtHaveModule(gui): qtHaveModule(widgets): SUBDIRS += quickwidgets

SUBDIRS += \
    plugins \
    imports \
    qmldevtools

!contains(QT_CONFIG, no-qml-debug): SUBDIRS += qmldebug

qmldevtools.CONFIG = host_build
