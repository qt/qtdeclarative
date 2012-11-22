TEMPLATE = subdirs
SUBDIRS =   calqlatr \
            samegame \
            tweetsearch \
            maroon \
            stocqt

# install
sources.files = demos.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/demos
INSTALLS += sources
