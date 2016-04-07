TEMPLATE = subdirs
SUBDIRS = qml script
contains(QT_CONFIG, private_tests) {
    SUBDIRS += particles
}
