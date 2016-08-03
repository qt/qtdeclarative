TEMPLATE = subdirs
SUBDIRS = samegame \
            calqlatr \
            clocks \
            tweetsearch \
            maroon \
            photosurface \
            photoviewer \
            stocqt

qtHaveModule(xmlpatterns): SUBDIRS += rssnews

