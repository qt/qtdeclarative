TEMPLATE = subdirs
SUBDIRS += \
    buttons \
    dialogs \
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
