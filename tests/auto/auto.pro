TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    cursor \
    drawer \
    focus \
    menu \
    palette \
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

# QTBUG-60268
boot2qt: SUBDIRS -= applicationwindow calendar controls cursor \
                    drawer focus menu platform palette popup \
                    qquickmaterialstyle qquickmaterialstyleconf \
                    qquickuniversalstyle qquickuniversalstyleconf \
                    snippets
