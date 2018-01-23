TEMPLATE = subdirs
QT_FOR_CONFIG += qml-private

# Utilities
SUBDIRS += \
    packetprotocol

# Connectors
SUBDIRS += \
    qmldbg_native \
    qmldbg_server

qmldbg_native.depends = packetprotocol
qmldbg_server.depends = packetprotocol

qtConfig(qml-network) {
    qtConfig(localserver): SUBDIRS += qmldbg_local

    SUBDIRS += \
        qmldbg_tcp
}

# Services
SUBDIRS += \
    qmldbg_messages \
    qmldbg_profiler \
    qmldbg_debugger \
    qmldbg_nativedebugger

qmldbg_messages.depends = packetprotocol
qmldbg_profiler.depends = packetprotocol
qmldbg_debugger.depends = packetprotocol
qmldbg_nativedebugger.depends = packetprotocol

qtHaveModule(quick) {
    SUBDIRS += \
        qmldbg_inspector \
        qmldbg_quickprofiler
    qmldbg_inspector.depends = packetprotocol
    qmldbg_quickprofiler.depends = packetprotocol
}
