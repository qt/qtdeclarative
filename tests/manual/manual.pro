TEMPLATE = subdirs
SUBDIRS += \
    gifs \
    fonts \
    testbench

qtHaveModule(widgets): SUBDIRS += viewinqwidget
