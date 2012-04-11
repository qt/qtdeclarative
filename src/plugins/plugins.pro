TEMPLATE = subdirs
SUBDIRS +=  qmltooling
contains(QT_CONFIG, accessibility) {
    SUBDIRS += accessible
}
