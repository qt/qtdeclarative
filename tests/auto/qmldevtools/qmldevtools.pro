TEMPLATE = subdirs

contains(QT_CONFIG, private_tests) {
    SUBDIRS += \
        compile
}

# Tests which should run in Pulse
PULSE_TESTS = $$SUBDIRS
