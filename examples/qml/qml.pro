TEMPLATE = subdirs

qtHaveModule(quick): SUBDIRS += networkaccessmanagerfactory xmlhttprequest

SUBDIRS += \
          plugins \
          referenceexamples \
          shell

EXAMPLE_FILES = \
    dynamicscene \
    i18n \
    locale
