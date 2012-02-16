TEMPLATE = subdirs

# These examples contain some C++ and need to be built
SUBDIRS = \
   cppextensions \
   minehunt \
   modelviews \
   painteditem \
   tutorials \
   script

# These examples contain no C++ and can simply be copied
sources.files = \
   animation \
   calculator \
   cppextensions \
   flickr \
   i18n \
   imageelements \
   keyinteraction \
   photoviewer \
   positioners \
   rssnews \
   samegame \
   snake \
   sqllocalstorage \
   text \
   threading \
   touchinteraction \
   toys \
   twitter \
   ui-components \
   webbrowser \
   xml


sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml
INSTALLS += sources
