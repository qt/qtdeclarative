TEMPLATE = subdirs
SUBDIRS += \
    templates \
    controls \
    imports

controls.depends = templates
imports.depends = controls templates
