CONFIG += tests_need_tools examples_need_tools

# Otherwise we cannot compile src/qmltyperegistrar
requires(qtConfig(commandlineparser))
requires(qtConfig(temporaryfile))

load(qt_parts)
