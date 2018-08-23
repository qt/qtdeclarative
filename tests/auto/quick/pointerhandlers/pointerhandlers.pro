TEMPLATE = subdirs

qtConfig(private_tests) {
    SUBDIRS += \
        flickableinterop \
        multipointtoucharea_interop \
        qquickdraghandler \
        qquickhoverhandler \
        qquickpinchhandler \
        qquickpointerhandler \
        qquickpointhandler \
        qquicktaphandler \
}
