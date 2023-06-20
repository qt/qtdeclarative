TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    sidepanel \
    swipetoremove \
    wearable \
    imagine/automotive

qtHaveModule(sql): SUBDIRS += eventcalendar
qtHaveModule(widgets): SUBDIRS += flatstyle
