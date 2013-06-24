TEMPLATE = subdirs
SUBDIRS = accessibility \
            animation \
            draganddrop \
            canvas \
            imageelements \
            keyinteraction \
            localstorage \
            models \
            views \
            mousearea \
            positioners \
            righttoleft \
            scenegraph \
            shadereffects \
            text \
            threading \
            touchinteraction \
            tutorials \
            customitems \
            imageprovider \
            window \
            dialogs \
            particles \
            demos

# Widget dependent examples
qtHaveModule(widgets) {
    SUBDIRS += embeddedinwidgets
}

EXAMPLE_FILES = \
    ui-components \
    shared
