TEMPLATE = subdirs

SUBDIRS =  qmldbg_tcp
!isEmpty(QT.quick.name): SUBDIRS += qmldbg_qtquick2
