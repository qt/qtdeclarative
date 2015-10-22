TEMPLATE = subdirs

PUBLICTESTS += \
    qdebugmessageservice \
    qqmlenginedebugservice \
    qqmldebugjs \
    qqmlinspector \
    qqmlprofilerservice \
    qpacketprotocol \
    qqmlenginedebuginspectorintegrationtest \
    qqmlenginecontrol \
    qqmldebuggingenabler \
    qqmlnativeconnector

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebuglocal \
    qqmldebugservice

SUBDIRS += $$PUBLICTESTS

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
