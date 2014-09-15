TEMPLATE = subdirs
!nacl:!contains(QT_CONFIG, no-qml-debug):SUBDIRS += qmltooling
!nacl:SUBDIRS +=  qmltooling
