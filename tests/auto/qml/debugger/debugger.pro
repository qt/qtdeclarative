TEMPLATE = subdirs

PUBLICTESTS += \
    qqmlenginedebugservice \
    qqmldebugjs \
    qqmlinspector \
    qqmlprofilerservice \
    qpacketprotocol \
    qqmlenginedebuginspectorintegrationtest \
    qqmlenginecontrol \
    qqmldebuggingenabler

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebuglocal \
    qqmldebugservice

SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
