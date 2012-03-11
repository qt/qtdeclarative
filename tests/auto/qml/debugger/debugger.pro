TEMPLATE = subdirs

PRIVATETESTS += \
    qqmlenginedebugservice \
    qqmldebugclient \
    qqmldebugservice \
    qqmldebugjs \
    qqmlinspector \
    qqmlprofilerservice \
    qpacketprotocol \
    qv8profilerservice \
    qdebugmessageservice

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
