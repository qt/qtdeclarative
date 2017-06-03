TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    cursor \
    drawer \
    focus \
    font \
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
    qquickmenubar \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    qquickuniversalstyleconf \
    revisions \
    sanity \
    snippets

# QTBUG-60268
boot2qt: SUBDIRS -= applicationwindow calendar controls cursor \
                    drawer focus font menu platform palette popup \
                    qquickmaterialstyle qquickmaterialstyleconf \
                    qquickuniversalstyle qquickuniversalstyleconf \
                    snippets qquickmenubar
