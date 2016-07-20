TEMPLATE = subdirs
SUBDIRS += \
    buttons \
    gifs \
    fonts \
    styles \
    testbench

qtHaveModule(widgets): SUBDIRS += viewinqwidget
