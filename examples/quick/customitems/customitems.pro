TEMPLATE = subdirs
SUBDIRS = \
    #dialcontrol \
    #flipable \
    painteditem \
    #progressbar \
    #scrollbar \
    #searchbox \
    #slideswitch \
    #spinner \
    #tabwidget \
    maskedmousearea

# install
sources.files = customitems.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/customitems
INSTALLS += sources
