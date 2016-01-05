TEMPLATE = subdirs
SUBDIRS += \
    templates \
    controls \
    imports

imports.depends = controls templates
