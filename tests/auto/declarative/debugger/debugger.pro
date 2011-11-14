TEMPLATE = subdirs

PRIVATETESTS += \
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
    qdeclarativedebugjs \
    qdeclarativeinspector \
    qdeclarativedebugtrace \
    qpacketprotocol

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
