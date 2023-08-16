TEMPLATE = subdirs
SUBDIRS =   quick-accessibility \
            animation \
            draganddrop \
            canvas \
            imageelements \
            keyinteraction \
            layouts \
            responsivelayouts \
            localstorage \
            models \
            views \
            tableview \
            mousearea \
            pointerhandlers \
            positioners \
            scenegraph \
            shadereffects \
            text \
            tutorials \
            customitems \
            imageprovider \
            imageresponseprovider \
            window \
            particles \
            delegatechooser \
            shapes \
            itemvariablerefreshrate \
            multieffect

#OpenGL Support Required
qtConfig(opengl(es1|es2)?) {
    SUBDIRS += \
    rendercontrol
}

# Widget dependent examples
qtHaveModule(widgets) {
    SUBDIRS += embeddedinwidgets
    qtHaveModule(quickwidgets):qtConfig(opengl(es1|es2)?): SUBDIRS += quickwidgets
}

EXAMPLE_FILES = \
    shared
