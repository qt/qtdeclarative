// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfontloader_p.h"

#include <qqmlcontext.h>
#include <qqmlengine.h>

#include <QStringList>
#include <QUrl>
#include <QDebug>

#include <QFontDatabase>

#include <private/qobject_p.h>
#include <qqmlinfo.h>
#include <qqmlfile.h>

#if QT_CONFIG(qml_network)
#include <QNetworkRequest>
#include <QNetworkReply>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/private/qduplicatetracker_p.h>

#include <QtGui/private/qfontdatabase_p.h>

QT_BEGIN_NAMESPACE

#define FONTLOADER_MAXIMUM_REDIRECT_RECURSION 16

class QQuickFontObject : public QObject
{
Q_OBJECT

public:
    explicit QQuickFontObject(int _id = -1);

#if QT_CONFIG(qml_network)
    void download(const QUrl &url, QNetworkAccessManager *manager);

Q_SIGNALS:
    void fontDownloaded(int id);

private:
    int redirectCount = 0;
    QNetworkReply *reply = nullptr;

private Q_SLOTS:
    void replyFinished();
#endif // qml_network

public:
    int id;

    Q_DISABLE_COPY(QQuickFontObject)
};

QQuickFontObject::QQuickFontObject(int _id)
    : QObject(nullptr), id(_id)
{
}

#if QT_CONFIG(qml_network)
void QQuickFontObject::download(const QUrl &url, QNetworkAccessManager *manager)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    reply = manager->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
}

void QQuickFontObject::replyFinished()
{
    if (reply) {
        redirectCount++;
        if (redirectCount < FONTLOADER_MAXIMUM_REDIRECT_RECURSION) {
            QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            if (redirect.isValid()) {
                QUrl url = reply->url().resolved(redirect.toUrl());
                QNetworkAccessManager *manager = reply->manager();
                reply->deleteLater();
                reply = nullptr;
                download(url, manager);
                return;
            }
        }
        redirectCount = 0;

        if (!reply->error()) {
            id = QFontDatabase::addApplicationFontFromData(reply->readAll());
            emit fontDownloaded(id);
        } else {
            qWarning("%s: Unable to load font '%s': %s", Q_FUNC_INFO,
                     qPrintable(reply->url().toString()), qPrintable(reply->errorString()));
            emit fontDownloaded(-1);
        }
        reply->deleteLater();
        reply = nullptr;
    }
}
#endif // qml_network

class QQuickFontLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQuickFontLoader)

public:
    QQuickFontLoaderPrivate() {}

    QUrl url;
    QFont font;
    QQuickFontLoader::Status status = QQuickFontLoader::Null;
};

static void q_QFontLoaderFontsStaticReset();
static void q_QFontLoaderFontsAddReset()
{
    qAddPostRoutine(q_QFontLoaderFontsStaticReset);
}
class QFontLoaderFonts
{
public:
    QFontLoaderFonts()
    {
        qAddPostRoutine(q_QFontLoaderFontsStaticReset);
        qAddPreRoutine(q_QFontLoaderFontsAddReset);
    }

    ~QFontLoaderFonts()
    {
        qRemovePostRoutine(q_QFontLoaderFontsStaticReset);
        reset();
    }


    void reset()
    {
        QDuplicateTracker<QQuickFontObject *, 256> deleted(map.size());
        for (QQuickFontObject *fo : std::as_const(map)) {
            if (!deleted.hasSeen(fo))
                delete fo;
        }
        map.clear();
    }

    QHash<QUrl, QQuickFontObject *> map;
};
Q_GLOBAL_STATIC(QFontLoaderFonts, fontLoaderFonts);

static void q_QFontLoaderFontsStaticReset()
{
    fontLoaderFonts()->reset();
}

/*!
    \qmltype FontLoader
    \instantiates QQuickFontLoader
    \inqmlmodule QtQuick
    \ingroup qtquick-text-utility
    \brief Allows fonts to be loaded by URL.

    The FontLoader type is used to load fonts by URL.

    The \l status indicates when the font has been loaded, which is useful
    for fonts loaded from remote sources.

    For example:
    \qml
    import QtQuick 2.0

    Column {
        FontLoader { id: webFont; source: "http://www.mysite.com/myfont.ttf" }

        Text { text: "Fancy font"; font: webFont.font }
    }
    \endqml

    \sa {Qt Quick Examples - Text#Fonts}{Qt Quick Examples - Text Fonts}
*/
QQuickFontLoader::QQuickFontLoader(QObject *parent)
    : QObject(*(new QQuickFontLoaderPrivate), parent)
{
    connect(this, &QQuickFontLoader::fontChanged, this, &QQuickFontLoader::nameChanged);
}

/*!
    \qmlproperty url QtQuick::FontLoader::source
    The URL of the font to load.
*/
QUrl QQuickFontLoader::source() const
{
    Q_D(const QQuickFontLoader);
    return d->url;
}

void QQuickFontLoader::setSource(const QUrl &url)
{
    Q_D(QQuickFontLoader);
    if (url == d->url)
        return;
    d->url = url;
    emit sourceChanged();

    const QQmlContext *context = qmlContext(this);
    const QUrl &resolvedUrl = context ? context->resolvedUrl(d->url) : d->url;
    QString localFile = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
    if (!localFile.isEmpty()) {
        if (!fontLoaderFonts()->map.contains(resolvedUrl)) {
            int id = QFontDatabase::addApplicationFont(localFile);
            updateFontInfo(id);
            if (id != -1) {
                QQuickFontObject *fo = new QQuickFontObject(id);
                fontLoaderFonts()->map[resolvedUrl] = fo;
            }
        } else {
            updateFontInfo(fontLoaderFonts()->map.value(resolvedUrl)->id);
        }
    } else {
        if (!fontLoaderFonts()->map.contains(resolvedUrl)) {
            Q_ASSERT(context);
#if QT_CONFIG(qml_network)
            QQuickFontObject *fo = new QQuickFontObject;
            fontLoaderFonts()->map[resolvedUrl] = fo;
            fo->download(resolvedUrl, context->engine()->networkAccessManager());
            d->status = Loading;
            emit statusChanged();
            QObject::connect(fo, SIGNAL(fontDownloaded(int)),
                this, SLOT(updateFontInfo(int)));
#else
// Silently fail if compiled with no_network
#endif
        } else {
            QQuickFontObject *fo = fontLoaderFonts()->map.value(resolvedUrl);
            if (fo->id == -1) {
#if QT_CONFIG(qml_network)
                d->status = Loading;
                emit statusChanged();
                QObject::connect(fo, SIGNAL(fontDownloaded(int)),
                    this, SLOT(updateFontInfo(int)));
#else
// Silently fail if compiled with no_network
#endif
            }
            else
                updateFontInfo(fo->id);
        }
    }
}

void QQuickFontLoader::updateFontInfo(int id)
{
    Q_D(QQuickFontLoader);

    QFont font;

    QQuickFontLoader::Status status = Error;
    if (id >= 0) {
        QFontDatabasePrivate *p = QFontDatabasePrivate::instance();
        if (id < p->applicationFonts.size()) {
            const QFontDatabasePrivate::ApplicationFont &applicationFont = p->applicationFonts.at(id);

            if (!applicationFont.properties.isEmpty()) {
                const QFontDatabasePrivate::ApplicationFont::Properties &properties = applicationFont.properties.at(0);
                font.setFamily(properties.familyName);
                font.setStyleName(properties.styleName);
                font.setWeight(QFont::Weight(properties.weight));
                font.setStyle(properties.style);
                font.setStretch(properties.stretch);
            }
        }

        status = Ready;
    }

    if (font != d->font) {
        d->font = font;
        emit fontChanged();
    }

    if (status != d->status) {
        if (status == Error) {
            const QQmlContext *context = qmlContext(this);
            qmlWarning(this) << "Cannot load font: \""
                             << (context ? context->resolvedUrl(d->url) : d->url).toString() << '"';
        }
        d->status = status;
        emit statusChanged();
    }
}

/*!
    \qmlproperty font QtQuick::FontLoader::font
    \since 6.0

    This property holds a default query for the loaded font.

    You can use this to select the font if other properties than just the
    family name are needed to disambiguate. You can either specify the
    font using individual properties:

    \qml
    Item {
        width: 200; height: 50

        FontLoader {
            id: webFont
            source: "http://www.mysite.com/myfont.ttf"
        }
        Text {
            text: "Fancy font"
            font.family: webFont.font.family
            font.weight: webFont.font.weight
            font.styleName: webFont.font.styleName
            font.pixelSize: 24
        }
    }
    \endqml

    Or you can set the full font query directly:

    \qml
    Item {
        width: 200; height: 50

        FontLoader {
            id: webFont
            source: "http://www.mysite.com/myfont.ttf"
        }
        Text {
            text: "Fancy font"
            font: webFont.font
        }
    }
    \endqml

    In this case, the default font query will be used with no modifications
    (so font size, for instance, will be the system default).
*/
QFont QQuickFontLoader::font() const
{
    Q_D(const QQuickFontLoader);
    return d->font;
}

/*!
    \qmlproperty string QtQuick::FontLoader::name
    \readonly

    This property holds the name of the font family.
    It is set automatically when a font is loaded using the \l source property.

    This is equivalent to the family property of the FontLoader's \l font property.

    Use this to set the \c font.family property of a \c Text item.

    Example:
    \qml
    Item {
        width: 200; height: 50

        FontLoader {
            id: webFont
            source: "http://www.mysite.com/myfont.ttf"
        }
        Text {
            text: "Fancy font"
            font.family: webFont.name
        }
    }
    \endqml
*/
QString QQuickFontLoader::name() const
{
    Q_D(const QQuickFontLoader);
    return d->font.resolveMask() == 0 ? QString() : d->font.family();
}

/*!
    \qmlproperty enumeration QtQuick::FontLoader::status

    This property holds the status of font loading.  It can be one of:

    \value FontLoader.Null      no font has been set
    \value FontLoader.Ready     the font has been loaded
    \value FontLoader.Loading   the font is currently being loaded
    \value FontLoader.Error     an error occurred while loading the font

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \li Trigger a state change:
    \qml
        State { name: 'loaded'; when: loader.status == FontLoader.Ready }
    \endqml

    \li Implement an \c onStatusChanged signal handler:
    \qml
        FontLoader {
            id: loader
            onStatusChanged: if (loader.status == FontLoader.Ready) console.log('Loaded')
        }
    \endqml

    \li Bind to the status value:
    \qml
        Text { text: loader.status == FontLoader.Ready ? 'Loaded' : 'Not loaded' }
    \endqml
    \endlist
*/
QQuickFontLoader::Status QQuickFontLoader::status() const
{
    Q_D(const QQuickFontLoader);
    return d->status;
}

QT_END_NAMESPACE

#include <qquickfontloader.moc>

#include "moc_qquickfontloader_p.cpp"
