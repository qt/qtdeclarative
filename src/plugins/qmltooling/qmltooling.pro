TEMPLATE = subdirs

# Connectors
SUBDIRS += \
    qmldbg_native \
    qmldbg_server \
        qmldbg_local \
        qmldbg_tcp

# Services
SUBDIRS += \
    qmldbg_debugger \
    qmldbg_profiler

qtHaveModule(quick): SUBDIRS += qmldbg_inspector
