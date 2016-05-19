TEMPLATE = subdirs
SUBDIRS =   quick-accessibility \
            animation \
            draganddrop \
            externaldraganddrop \
            canvas \
            imageelements \
            keyinteraction \
            layouts \
            localstorage \
            models \
            views \
            mousearea \
            positioners \
            righttoleft \
            scenegraph \
            shadereffects \
            text \
            textureprovider \
            threading \
            touchinteraction \
            tutorials \
            customitems \
            imageprovider \
            imageresponseprovider \
            window \
            particles \
            demos \
            rendercontrol

# Widget dependent examples
qtHaveModule(widgets) {
    SUBDIRS += embeddedinwidgets
    qtHaveModule(quickwidgets): SUBDIRS += quickwidgets
}

EXAMPLE_FILES = \
    shared
