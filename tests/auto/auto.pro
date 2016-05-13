TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    drawer \
    material \
    menu \
    popup \
    pressandhold \
    qquickstyle \
    qquickstyleselector \
    sanity \
    snippets \
    styles \
    universal

# QTBUG-50295
!linux: SUBDIRS += \
    focus
