OTHER_FILES += \
               $$PWD/qtquick.qdocconf \
               $$PWD/qtquick-dita.qdocconf

online_docs.commands = qdoc3 $$PWD/qtquick.qdocconf

dita_docs.commands = qdoc3 $$PWD/qtquick-dita.qdocconf

docs.depends = dita_docs online_docs
QMAKE_EXTRA_TARGETS = docs dita_docs online_docs
QMAKE_CLEAN += \
               "-r $$PWD/html" \
               "-r $$PWD/ditaxml"