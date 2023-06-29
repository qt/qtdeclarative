TEMPLATE = subdirs
SUBDIRS =   quick-accessibility \
            animation \
            draganddrop \
            externaldraganddrop \
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
            righttoleft \
            scenegraph \
            shadereffects \
            text \
            threading \
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
