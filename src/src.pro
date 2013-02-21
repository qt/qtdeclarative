TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    qml

qtHaveModule(gui):contains(QT_CONFIG, opengl(es1|es2)?) {
    SUBDIRS += \
        quick \
        qmltest \
        particles
}

SUBDIRS += \
    plugins \
    imports \
    qmldevtools
