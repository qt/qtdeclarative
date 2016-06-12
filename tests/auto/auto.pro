TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    drawer \
    menu \
    popup \
    pressandhold \
    qquickmaterialstyle \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    sanity \
    snippets \
    styles

# QTBUG-50295
!linux: SUBDIRS += \
    focus
