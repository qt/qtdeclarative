TEMPLATE = subdirs

qtConfig(private_tests) {
    SUBDIRS += \
        flickableinterop \
        multipointtoucharea_interop \
        qquickpointerhandler \
        qquickdraghandler \
        qquicktaphandler \
}

