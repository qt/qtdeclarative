TEMPLATE = subdirs
SUBDIRS = qml script
contains(QT_CONFIG, private_tests) {
    contains(QT_CONFIG, opengl(es1|es2)?):SUBDIRS += particles
}
