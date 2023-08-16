TEMPLATE = subdirs
SUBDIRS += \
    buttons \
    fonts \
    gifs \
    headerview \
    imagine/musicplayer \
    qquickdialog \
    screenshots \
    styles \
    styles-cover-flow \
    systemtrayicon \
    testbench \
    viewinqwidget

qtConfig(systemtrayicon): SUBDIRS += systemtrayicon

qtHaveModule(widgets): SUBDIRS += viewinqwidget
