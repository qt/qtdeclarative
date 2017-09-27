TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    applicationwindow \
    calendar \
    controls \
    cursor \
    focus \
    font \
    menu \
    platform \
    popup \
    pressandhold \
    qquickdrawer \
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
                    qquickdrawer focus font menu platform popup qquickmaterialstyle \
                    qquickmaterialstyleconf qquickuniversalstyle \
                    qquickuniversalstyleconf snippets
