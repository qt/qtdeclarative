TEMPLATE = subdirs

PRIVATETESTS += \
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
    qdeclarativedebugjs \
    qdeclarativeinspector \
    qdeclarativedebugtrace \
    qpacketprotocol \
    qv8profilerservice

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
