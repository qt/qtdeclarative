TEMPLATE = subdirs
SUBDIRS += \
    accessibility \
    calendar \
    controls \
    cursor \
    customization \
    designer \
    focus \
    font \
    palette \
    platform \
    pressandhold \
    qquickapplicationwindow \
    qquickcolor \
    qquickdrawer \
    qquickiconimage \
    qquickiconlabel \
    qquickimaginestyle \
    qquickmaterialstyle \
    qquickmaterialstyleconf \
    qquickmenu \
    qquickmenubar \
    qquickninepatchimage \
    qquickpopup \
    qquickstyle \
    qquickstyleselector \
    qquickuniversalstyle \
    qquickuniversalstyleconf \
    revisions \
    sanity \
    snippets

# Requires lrelease, which isn't always available in CI.
qtHaveModule(tools): translation
