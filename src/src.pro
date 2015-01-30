TEMPLATE = subdirs
SUBDIRS += \
    controls \
    calendar \
    extras \
    imports

calendar.depends = controls
extras.depends = controls
imports.depends = controls calendar extras
