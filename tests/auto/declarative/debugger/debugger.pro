TEMPLATE = subdirs

PRIVATETESTS += \
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
    qdeclarativedebugjs \
    qdeclarativeinspector \
    qpacketprotocol

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
