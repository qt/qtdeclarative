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
    qquickmaterialstyleconf \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    qquickuniversalstyleconf \
    sanity \
    snippets

# QTBUG-50295
!linux: SUBDIRS += \
    focus
