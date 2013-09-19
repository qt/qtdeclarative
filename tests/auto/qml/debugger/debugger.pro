TEMPLATE = subdirs

PUBLICTESTS += \
    qqmlenginedebugservice \
#    qqmldebugjs \
    qpacketprotocol \
#    qv8profilerservice \
#    qdebugmessageservice \
    qqmlenginedebuginspectorintegrationtest

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebugservice

!mac {
PUBLICTESTS += \
    qqmlinspector \
    qqmlprofilerservice
}

SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
