TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    wearable \
    imagine/automotive

win32|macos|unix {
    qtHaveModule(svg): SUBDIRS += filesystemexplorer
}

qtHaveModule(sql): SUBDIRS += eventcalendar
qtHaveModule(widgets): SUBDIRS += flatstyle
