TEMPLATE = subdirs
SUBDIRS += \
    buttons \
    fonts \
    gifs \
    headerview \
    qquickdialog \
    screenshots \
    styles \
    styles-cover-flow \
    systemtrayicon \
    testbench \
    viewinqwidget

qtConfig(systemtrayicon): SUBDIRS += systemtrayicon

qtHaveModule(widgets): SUBDIRS += viewinqwidget
