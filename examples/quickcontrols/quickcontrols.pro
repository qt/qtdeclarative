TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    wearable \
    imagine/automotive \
    imagine/musicplayer

qtHaveModule(sql): SUBDIRS += eventcalendar
qtHaveModule(widgets): SUBDIRS += flatstyle
