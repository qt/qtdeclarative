TEMPLATE = subdirs

PRIVATETESTS += \
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
    qdeclarativedebugjs \
    qdeclarativeinspector \
    qdeclarativedebugtrace \
    qpacketprotocol \
    qv8profilerservice \
    qdebugmessageservice

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
