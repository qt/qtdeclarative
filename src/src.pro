TEMPLATE = subdirs
SUBDIRS += \
    controls \
    extras \
    imports

extras.depends = controls
imports.depends = controls extras
