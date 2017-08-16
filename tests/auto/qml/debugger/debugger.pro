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
    qqmlnativeconnector \
    qqmldebugprocess

PRIVATETESTS += \
    qqmldebugclient \
    qqmldebuglocal \
    qqmldebugservice \
    qv4debugger

SUBDIRS += $$PUBLICTESTS

qtConfig(private_tests): \
    SUBDIRS += $$PRIVATETESTS

