OTHER_FILES += \
               $$PWD/qtquick.qdocconf \
               $$PWD/qtquick-dita.qdocconf

QDOC = $$QT.core.bins/qdoc

ONLINE_CONF = $$PWD/qtquick.qdocconf
DITA_CONF = $$PWD/qtquick-dita.qdocconf
QCH_CONF = #nothing yet

$$unixstyle {
} else {
    QDOC = $$replace(QDOC, "qdoc", "qdoc3.exe")
    ONLINE_CONF = $$replace(ONLINE_CONF, "/", "\\")
    DITA_DOCS = $$replace(ONLINE_CONF, "/", "\\")
}

qtquick_online_docs.commands = $$QDOC $$ONLINE_CONF
qtquick_dita_docs.commands = $$QDOC $$DITA_CONF

qtquick_docs.depends = qtquick_dita_docs qtquick_online_docs
QMAKE_EXTRA_TARGETS = qtquick_docs qtquick_dita_docs qtquick_online_docs
QMAKE_CLEAN += \
               "-r $$PWD/html" \
               "-r $$PWD/ditaxml"
