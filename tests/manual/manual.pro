TEMPLATE = subdirs
SUBDIRS += \
    gifs \
    fonts \
    styles \
    testbench

qtHaveModule(widgets): SUBDIRS += viewinqwidget
