TEMPLATE = subdirs
SUBDIRS += \
    controls \
    imports

imports.depends = controls
