TEMPLATE = subdirs
SUBDIRS += \
    controls \
    imports \
    templates

imports.depends = controls templates
