TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    drawer \
    menu \
    platform \
    popup \
    pressandhold \
    qquickmaterialstyle \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    sanity \
    snippets

# QTBUG-50295
!linux: SUBDIRS += \
    focus
