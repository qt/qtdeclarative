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

!contains(QT_CONFIG, no-widgets): SUBDIRS += extended
