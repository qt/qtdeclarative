TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    calendar \
    controls \
    cursor \
    focus \
    font \
    menu \
    platform \
    pressandhold \
    qquickapplicationwindow \
    qquickdrawer \
    qquickmaterialstyle \
    qquickmaterialstyleconf \
    qquickpopup \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    qquickuniversalstyleconf \
    revisions \
    sanity \
    snippets

# QTBUG-60268
boot2qt: SUBDIRS -= qquickapplicationwindow calendar controls cursor \
                    qquickdrawer focus font menu platform qquickpopup qquickmaterialstyle \
                    qquickmaterialstyleconf qquickuniversalstyle \
                    qquickuniversalstyleconf snippets
