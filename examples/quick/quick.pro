TEMPLATE = subdirs
SUBDIRS = accessibility \
            animation \
            draganddrop \
            canvas \
            imageelements \
            keyinteraction \
            modelviews \
            mousearea \
            positioners \
            righttoleft \
            scenegraph \
            shadereffects \
            text \
            threading \
            touchinteraction \
            customitems

# install
sources.files = quick.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick
INSTALLS += sources
