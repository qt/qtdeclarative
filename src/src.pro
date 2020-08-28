TEMPLATE = subdirs
SUBDIRS += \
    quicktemplates2 \
    quickcontrols2 \
    quickcontrols2impl \
    imports

quickcontrols2.depends = quicktemplates2
quickcontrols2impl.depends = quicktemplates2
imports.depends = quicktemplates2 quickcontrols2 quickcontrols2impl
