TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    cursor \
    drawer \
    menu \
    platform \
    popup \
    pressandhold \
    qquickcolor \
    qquickiconimage \
    qquickiconlabel \
    qquickmaterialstyle \
    qquickmaterialstyleconf \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    qquickuniversalstyleconf \
    revisions \
    sanity \
    snippets

# QTBUG-50295
!linux: SUBDIRS += \
    focus
