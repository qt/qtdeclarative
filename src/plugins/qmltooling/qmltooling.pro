TEMPLATE = subdirs

# Connectors
SUBDIRS += \
    qmldbg_native \
    qmldbg_server \
        qmldbg_local \
        qmldbg_tcp

# Services
SUBDIRS += \
    packetprotocol \
    qmldbg_debugger \
    qmldbg_profiler

qmldbg_server.depends = packetprotocol

qtHaveModule(quick): SUBDIRS += \
    qmldbg_inspector \
    qmldbg_quickprofiler
