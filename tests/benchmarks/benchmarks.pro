TEMPLATE = subdirs
SUBDIRS = qml
contains(QT_CONFIG, private_tests) {
    SUBDIRS += particles
}
