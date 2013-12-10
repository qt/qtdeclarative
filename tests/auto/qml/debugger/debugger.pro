TEMPLATE = subdirs

PUBLICTESTS += \
    qqmlenginedebugservice \
    qqmldebugjs \
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
