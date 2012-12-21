TEMPLATE = subdirs

SUBDIRS += \
    adding \
    attached \
    binding \
    coercion \
    default \
    grouped \
    properties \
    signal \
    valuesource \
    methods

qtHaveModule(widgets): SUBDIRS += extended
