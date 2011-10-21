TEMPLATE = subdirs

PRIVATETESTS += \
    qdeclarativeenginedebug \
    qdeclarativedebugclient \
    qdeclarativedebugservice \
    qdeclarativedebugjs \
    qpacketprotocol

contains(QT_CONFIG, private_tests) {
    SUBDIRS += $$PRIVATETESTS
}
