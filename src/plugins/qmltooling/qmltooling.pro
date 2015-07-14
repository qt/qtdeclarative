TEMPLATE = subdirs

SUBDIRS += \
    qmldbg_local \
    qmldbg_server \
    qmldbg_tcp

qtHaveModule(quick): SUBDIRS += qmldbg_qtquick2
