TEMPLATE = subdirs

SUBDIRS = \
    application.pro \
    library

application.depends = library
