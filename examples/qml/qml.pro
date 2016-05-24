TEMPLATE = subdirs

qtHaveModule(quick) {
    SUBDIRS += \
        qmlextensionplugins \
        xmlhttprequest

    !no_network: SUBDIRS += \
            networkaccessmanagerfactory
}

SUBDIRS += \
          referenceexamples \
          tutorials \
          shell

EXAMPLE_FILES = \
    dynamicscene \
    qml-i18n \
    locale
