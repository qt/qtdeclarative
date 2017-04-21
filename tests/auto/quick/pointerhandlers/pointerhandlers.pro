TEMPLATE = subdirs

qtConfig(private_tests) {
    SUBDIRS += \
        flickableinterop \
        qquickpointerhandler \
        qquicktaphandler \
}

