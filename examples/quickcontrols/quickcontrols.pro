TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    wearable \
    filesystemexplorer \
    imagine/automotive

qtHaveModule(sql): SUBDIRS += eventcalendar
qtHaveModule(widgets): SUBDIRS += flatstyle
