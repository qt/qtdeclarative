TEMPLATE = subdirs

PUBLICTESTS += \
    qqmlenginedebugservice \
    qqmldebugjs \
    qqmlinspector \
    qqmlprofilerservice \
    qpacketprotocol \
    qv8profilerservice \
    qdebugmessageservice

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebugservice

SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
