TEMPLATE = subdirs

PUBLICTESTS += \
    qqmlenginedebugservice \
    qqmldebugjs \
    qqmlinspector \
    qqmlprofilerservice \
    qpacketprotocol \
#    qv4profilerservice \
#    qdebugmessageservice \
    qqmlenginedebuginspectorintegrationtest \
    qqmlinspector \
    qqmlprofilerservice

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebugservice

SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
