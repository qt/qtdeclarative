// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlfile.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QTranslator>
#include <QQmlComponent>
#include "qqmlapplicationengine.h"
#include "qqmlapplicationengine_p.h"
#include <QtQml/private/qqmlcomponent_p.h>
#include <QtQml/private/qqmldirdata_p.h>
#include <QtQml/private/qqmlfileselector_p.h>

QT_BEGIN_NAMESPACE

QQmlApplicationEnginePrivate::QQmlApplicationEnginePrivate(QQmlEngine *e)
    : QQmlEnginePrivate(e)
{
    uiLanguage = QLocale().bcp47Name();
}

QQmlApplicationEnginePrivate::~QQmlApplicationEnginePrivate()
{
}

void QQmlApplicationEnginePrivate::ensureInitialized()
{
    if (!isInitialized) {
        init();
        isInitialized = true;
    }
}

void QQmlApplicationEnginePrivate::cleanUp()
{
    Q_Q(QQmlApplicationEngine);
    for (auto obj : std::as_const(objects))
        obj->disconnect(q);

    qDeleteAll(objects);
}

void QQmlApplicationEnginePrivate::init()
{
    Q_Q(QQmlApplicationEngine);
    q->connect(q, &QQmlApplicationEngine::quit, QCoreApplication::instance(),
               &QCoreApplication::quit, Qt::QueuedConnection);
    q->connect(q, &QQmlApplicationEngine::exit, QCoreApplication::instance(),
               &QCoreApplication::exit, Qt::QueuedConnection);
    QObject::connect(q, &QJSEngine::uiLanguageChanged, q, [this](){
        _q_loadTranslations();
    });
#if QT_CONFIG(translation)
    QTranslator* qtTranslator = new QTranslator(q);
    if (qtTranslator->load(QLocale(), QLatin1String("qt"), QLatin1String("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath), QLatin1String(".qm")))
        QCoreApplication::installTranslator(qtTranslator);
    else
        delete qtTranslator;
#endif
    auto *selector = new QQmlFileSelector(q,q);
    selector->setExtraSelectors(extraFileSelectors);
    QCoreApplication::instance()->setProperty("__qml_using_qqmlapplicationengine", QVariant(true));
}

void QQmlApplicationEnginePrivate::_q_loadTranslations()
{
#if QT_CONFIG(translation)
    Q_Q(QQmlApplicationEngine);
    if (translationsDirectory.isEmpty())
        return;

    auto translator = std::make_unique<QTranslator>();
    if (!uiLanguage.value().isEmpty()) {
        QLocale locale(uiLanguage);
        if (translator->load(locale, QLatin1String("qml"), QLatin1String("_"), translationsDirectory, QLatin1String(".qm"))) {
            if (activeTranslator)
                QCoreApplication::removeTranslator(activeTranslator.get());
            QCoreApplication::installTranslator(translator.get());
            activeTranslator.swap(translator);
        }
    } else {
        activeTranslator.reset();
    }
    q->retranslate();
#endif
}

void QQmlApplicationEnginePrivate::startLoad(const QUrl &url, const QByteArray &data, bool dataFlag)
{
    Q_Q(QQmlApplicationEngine);

    ensureInitialized();

    if (url.scheme() == QLatin1String("file") || url.scheme() == QLatin1String("qrc")) {
        QFileInfo fi(QQmlFile::urlToLocalFileOrQrc(url));
        translationsDirectory = fi.path() + QLatin1String("/i18n");
    } else {
        translationsDirectory.clear();
    }

    _q_loadTranslations(); //Translations must be loaded before the QML file is
    QQmlComponent *c = new QQmlComponent(q, q);

    if (dataFlag)
        c->setData(data,url);
    else
        c->loadUrl(url);

    ensureLoadingFinishes(c);
}

void QQmlApplicationEnginePrivate::startLoad(QAnyStringView uri, QAnyStringView typeName)
{
    Q_Q(QQmlApplicationEngine);

    QQmlComponent *c = new QQmlComponent(q, q);

    ensureInitialized();

    auto *componentPriv = QQmlComponentPrivate::get(c);
    const auto [status, type] = componentPriv->prepareLoadFromModule(uri, typeName);

    if (type.sourceUrl().isValid()) {
        const auto qmlDirData = typeLoader.getQmldir(type.sourceUrl());
        const QUrl url = qmlDirData->finalUrl();
        if (url.scheme() == QLatin1String("file") || url.scheme() == QLatin1String("qrc")) {
            QFileInfo fi(QQmlFile::urlToLocalFileOrQrc(url));
            translationsDirectory = fi.path() + QLatin1String("/i18n");
        } else {
            translationsDirectory.clear();
        }
    }

    /* Translations must be loaded before the QML file. They require translationDirectory to
     * already be resolved. But, in order to resolve the translationDirectory, the type of the
     * module to load needs to be known. Therefore, loadFromModule is split into resolution and
     * loading because the translation directory needs to be set in between.
     */
    _q_loadTranslations();
    componentPriv->completeLoadFromModule(uri, typeName, type, status);

    ensureLoadingFinishes(c);
}

void QQmlApplicationEnginePrivate::finishLoad(QQmlComponent *c)
{
    Q_Q(QQmlApplicationEngine);
    switch (c->status()) {
    case QQmlComponent::Error:
        qWarning() << "QQmlApplicationEngine failed to load component";
        warning(c->errors());
        q->objectCreated(nullptr, c->url());
        q->objectCreationFailed(c->url());
        break;
    case QQmlComponent::Ready: {
        auto newObj = initialProperties.empty() ? c->create() : c->createWithInitialProperties(initialProperties);

        if (c->isError()) {
           qWarning() << "QQmlApplicationEngine failed to create component";
           warning(c->errors());
           q->objectCreated(nullptr, c->url());
           q->objectCreationFailed(c->url());
           break;
        }

        objects << newObj;
        QObject::connect(newObj, &QObject::destroyed, q, [&](QObject *obj) { objects.removeAll(obj); });
        q->objectCreated(objects.constLast(), c->url());
        }
        break;
    case QQmlComponent::Loading:
    case QQmlComponent::Null:
        return; //These cases just wait for the next status update
    }

    c->deleteLater();
}

void QQmlApplicationEnginePrivate::ensureLoadingFinishes(QQmlComponent *c)
{
    Q_Q(QQmlApplicationEngine);
    if (!c->isLoading()) {
        finishLoad(c);
        return;
    }
    QObject::connect(c, &QQmlComponent::statusChanged, q, [this, c] { this->finishLoad(c); });
}

/*!
  \class QQmlApplicationEngine
  \since 5.1
  \inmodule QtQml
  \brief QQmlApplicationEngine provides a convenient way to load an application from a single QML file.

  This class combines a QQmlEngine and QQmlComponent to provide a convenient way to load a single QML file. It also exposes some central application functionality to QML, which a C++/QML hybrid application would normally control from C++.

  It can be used like so:

  \code
  #include <QGuiApplication>
  #include <QQmlApplicationEngine>

  int main(int argc, char *argv[])
  {
      QGuiApplication app(argc, argv);
      QQmlApplicationEngine engine("main.qml");
      return app.exec();
  }
  \endcode

  Unlike QQuickView, QQmlApplicationEngine does not automatically create a root
  window. If you are using visual items from Qt Quick, you will need to place
  them inside of a \l [QML] {Window}.

  You can also use QCoreApplication with QQmlApplicationEngine, if you are not using any QML modules which require a QGuiApplication (such as \c QtQuick).

  List of configuration changes from a default QQmlEngine:

  \list
  \li Connecting Qt.quit() to QCoreApplication::quit()
  \li Automatically loads translation files from an i18n directory adjacent to the main QML file.
  \li Translations are reloaded when the \c QJSEngine::uiLanguage / \c Qt.uiLanguage property is changed.
  \li Automatically sets an incubation controller if the scene contains a QQuickWindow.
  \li Automatically sets a \c QQmlFileSelector as the url interceptor, applying file selectors to all
  QML files and assets.
  \endlist

  The engine behavior can be further tweaked by using the inherited methods from QQmlEngine.

  \note Translation files must have a \e qml_ prefix in order to be recognized,
        e.g. \e{qml_ja_JP.qm}.

  \note Placing translation files relative to the main QML file involves adding
        a \e RESOURCE_PREFIX to the relevant \l qt_add_translations call. This
        needs to include the resource prefix of the main file's QML module
        (\e{/qt/qml} by default) and the module URI. For example, to provide
        translation files for a module called "Translated":
        \snippet qml-i18n/CMakeLists.txt 0
*/

/*!
  \fn QQmlApplicationEngine::objectCreated(QObject *object, const QUrl &url)

  This signal is emitted when an object finishes loading. If loading was
  successful, \a object contains a pointer to the loaded object, otherwise
  the pointer is NULL.

  The \a url to the component the \a object came from is also provided.

  \note If the path to the component was provided as a QString containing a
  relative path, the \a url will contain a fully resolved path to the file.
*/

/*!
  \fn QQmlApplicationEngine::objectCreationFailed(const QUrl &url)
  \since 6.4

  This signal is emitted when loading finishes because an error occurred.

  The \a url to the component that failed to load is provided as an argument.

  \code
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // exit on error
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl());
    return app.exec();
  \endcode

  \note If the path to the component was provided as a QString containing a
  relative path, the \a url will contain a fully resolved path to the file.

  See also \l {QQmlApplicationEngine::objectCreated}, which will be emitted in
  addition to this signal (even though creation failed).
*/

/*!
  Create a new QQmlApplicationEngine with the given \a parent. You will have to call load() later in
  order to load a QML file.
*/
QQmlApplicationEngine::QQmlApplicationEngine(QObject *parent)
: QQmlEngine(*(new QQmlApplicationEnginePrivate(this)), parent)
{
    QJSEnginePrivate::addToDebugServer(this);
}

/*!
  Create a new QQmlApplicationEngine and loads the QML file at the given \a url.
  This is provided as a convenience,  and is the same as using the empty constructor and calling load afterwards.
*/
QQmlApplicationEngine::QQmlApplicationEngine(const QUrl &url, QObject *parent)
    : QQmlApplicationEngine(parent)
{
    load(url);
}

/*!
  Create a new QQmlApplicationEngine and loads the QML type specified by
  \a uri and \a typeName
  This is provided as a convenience,  and is the same as using the empty constructor and calling
  loadFromModule afterwards.

  \since 6.5
*/
QQmlApplicationEngine::QQmlApplicationEngine(QAnyStringView uri, QAnyStringView typeName, QObject *parent)
    : QQmlApplicationEngine(parent)
{
    loadFromModule(uri, typeName);
}

static QUrl urlFromFilePath(const QString &filePath)
{
    return filePath.startsWith(QLatin1Char(':'))
        ? QUrl(QLatin1String("qrc") + filePath)
        : QUrl::fromUserInput(filePath, QLatin1String("."), QUrl::AssumeLocalFile);
}

/*!
  Create a new QQmlApplicationEngine and loads the QML file at the given
  \a filePath, which must be a local file or qrc path. If a relative path is
  given then it will be interpreted as relative to the working directory of the
  application.

  This is provided as a convenience, and is the same as using the empty constructor and calling load afterwards.
*/
QQmlApplicationEngine::QQmlApplicationEngine(const QString &filePath, QObject *parent)
    : QQmlApplicationEngine(urlFromFilePath(filePath), parent)
{
}

/*!
  Destroys the QQmlApplicationEngine and all QML objects it loaded.
*/
QQmlApplicationEngine::~QQmlApplicationEngine()
{
    Q_D(QQmlApplicationEngine);
    QJSEnginePrivate::removeFromDebugServer(this);
    d->cleanUp();//Instantiated root objects must be deleted before the engine
}

/*!
  Loads the root QML file located at \a url. The object tree defined by the file
  is created immediately for local file urls. Remote urls are loaded asynchronously,
  listen to the \l {QQmlApplicationEngine::objectCreated()}{objectCreated} signal to
  determine when the object tree is ready.

  If an error occurs, the \l {QQmlApplicationEngine::objectCreated()}{objectCreated}
  signal is emitted with a null pointer as parameter and error messages are printed
  with qWarning.
*/
void QQmlApplicationEngine::load(const QUrl &url)
{
    Q_D(QQmlApplicationEngine);
    d->startLoad(url);
}

/*!
  Loads the root QML file located at \a filePath. \a filePath must be a path to
  a local file or a path to a file in the resource file system. If \a filePath
  is a relative path, it is taken as relative to the application's working
  directory. The object tree defined by the file is instantiated immediately.

  If an error occurs, error messages are printed with qWarning.
*/
void QQmlApplicationEngine::load(const QString &filePath)
{
    Q_D(QQmlApplicationEngine);
    d->startLoad(urlFromFilePath(filePath));
}

/*!
    Loads the QML type \a typeName from the module specified by \a uri.
    If the type originates from a QML file located at a  remote url,
    the type will be loaded asynchronously.
    Listen to the \l {QQmlApplicationEngine::objectCreated()}{objectCreated}
    signal to determine when the object tree is ready.

    If an error occurs, the \l {QQmlApplicationEngine::objectCreated()}{objectCreated}
    signal is emitted with a null pointer as parameter and error messages are printed
    with qWarning.

    \code
    QQmlApplicationEngine engine;
    engine.loadFromModule("QtQuick", "Rectangle");
    \endcode

    \note The module identified by \a uri is searched in the
    \l {QML Import Path}{import path}, in the same way as if
    you were doing \c{import uri} inside a QML file. If the
    module cannot be located there, this function will fail.

    \since 6.5
    \sa QQmlComponent::loadFromModule
 */
void QQmlApplicationEngine::loadFromModule(QAnyStringView uri, QAnyStringView typeName)
{
    Q_D(QQmlApplicationEngine);
    d->startLoad(uri, typeName);
}

/*!
   Sets the \a initialProperties with which the QML component gets initialized after
   it gets loaded.

   \code
    QQmlApplicationEngine engine;

    EventDatabase eventDatabase;
    EventMonitor eventMonitor;

    engine.setInitialProperties({
        { "eventDatabase", QVariant::fromValue(&eventDatabase) },
        { "eventMonitor", QVariant::fromValue(&eventMonitor) }
    });
   \endcode

   \sa QQmlComponent::setInitialProperties
   \sa QQmlApplicationEngine::load
   \sa QQmlApplicationEngine::loadData
   \since 5.14
*/
void QQmlApplicationEngine::setInitialProperties(const QVariantMap &initialProperties)
{
    Q_D(QQmlApplicationEngine);
    d->initialProperties = initialProperties;
}

/*!
  Sets the \a extraFileSelectors to be passed to the internal QQmlFileSelector
  used for resolving URLs to local files. The \a extraFileSelectors are applied
  when the first QML file is loaded. Setting them afterwards has no effect.

  \sa QQmlFileSelector
  \sa QFileSelector::setExtraSelectors
  \since 6.0
*/
void QQmlApplicationEngine::setExtraFileSelectors(const QStringList &extraFileSelectors)
{
    Q_D(QQmlApplicationEngine);
    if (d->isInitialized) {
        qWarning() << "QQmlApplicationEngine::setExtraFileSelectors()"
                   << "called after loading QML files. This has no effect.";
    } else {
        d->extraFileSelectors = extraFileSelectors;
    }
}

/*!
  Loads the QML given in \a data. The object tree defined by \a data is
  instantiated immediately.

  If a \a url is specified it is used as the base url of the component. This affects
  relative paths within the data and error messages.

  If an error occurs, error messages are printed with qWarning.
*/
void QQmlApplicationEngine::loadData(const QByteArray &data, const QUrl &url)
{
    Q_D(QQmlApplicationEngine);
    d->startLoad(url, data, true);
}

/*!
  Returns a list of all the root objects instantiated by the
  QQmlApplicationEngine. This will only contain objects loaded via load() or a
  convenience constructor.

  \note In Qt versions prior to 5.9, this function is marked as non-\c{const}.
*/

QList<QObject *> QQmlApplicationEngine::rootObjects() const
{
    Q_D(const QQmlApplicationEngine);
    return d->objects;
}

QT_END_NAMESPACE

#include "moc_qqmlapplicationengine.cpp"
