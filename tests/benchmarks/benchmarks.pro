TEMPLATE = subdirs
SUBDIRS = qml quick
qtConfig(private_tests) {
    SUBDIRS += particles
}
