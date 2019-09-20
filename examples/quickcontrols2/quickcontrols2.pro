TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    sidepanel \
    swipetoremove \
    wearable \
    imagine/automotive \
    imagine/musicplayer

qtHaveModule(widgets): SUBDIRS += flatstyle
