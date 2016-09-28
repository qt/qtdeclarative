TEMPLATE = subdirs
SUBDIRS += \
    gifs \
    fonts \
    screenshots \
    styles \
    testbench

qtHaveModule(widgets): SUBDIRS += viewinqwidget
