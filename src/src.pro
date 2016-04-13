TEMPLATE = subdirs
SUBDIRS += \
    quicktemplates2 \
    controls \
    imports

controls.depends = quicktemplates2
imports.depends = controls quicktemplates2
