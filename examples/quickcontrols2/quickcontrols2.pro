TEMPLATE = subdirs
SUBDIRS += \
    gallery \
    chattutorial \
    texteditor \
    contactlist \
    sidepanel \
    swipetoremove \
    wearable

qtHaveModule(widgets): SUBDIRS += flatstyle
