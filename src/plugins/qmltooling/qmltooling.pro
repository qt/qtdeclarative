TEMPLATE = subdirs

SUBDIRS =  qmldbg_tcp qmldbg_local
qtHaveModule(quick): SUBDIRS += qmldbg_qtquick2
