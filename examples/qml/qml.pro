TEMPLATE = subdirs

qtHaveModule(quick): SUBDIRS += networkaccessmanagerfactory xmlhttprequest

SUBDIRS += \
          qmlextensionplugins \
          referenceexamples \
          shell

EXAMPLE_FILES = \
    dynamicscene \
    qml-i18n \
    locale
