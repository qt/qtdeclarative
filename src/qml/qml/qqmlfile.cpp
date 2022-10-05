// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlfile.h"

#include <QtCore/qurl.h>
#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qfile.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>

/*!
\class QQmlFile
\brief The QQmlFile class gives access to local and remote files.

\internal

Supports file:// and qrc:/ uris and whatever QNetworkAccessManager supports.
*/

#define QQMLFILE_MAX_REDIRECT_RECURSION 16

QT_BEGIN_NAMESPACE

static char qrc_string[] = "qrc";
static char file_string[] = "file";

#if defined(Q_OS_ANDROID)
static char assets_string[] = "assets";
static char content_string[] = "content";
static char authority_externalstorage[] = "com.android.externalstorage.documents";
static char authority_downloads_documents[] = "com.android.providers.downloads.documents";
static char authority_media_documents[] = "com.android.providers.media.documents";
#endif

class QQmlFilePrivate;

#if QT_CONFIG(qml_network)
class QQmlFileNetworkReply : public QObject
{
Q_OBJECT
public:
    QQmlFileNetworkReply(QQmlEngine *, QQmlFilePrivate *, const QUrl &);
    ~QQmlFileNetworkReply();

signals:
    void finished();
    void downloadProgress(qint64, qint64);

public slots:
    void networkFinished();
    void networkDownloadProgress(qint64, qint64);

public:
    static int finishedIndex;
    static int downloadProgressIndex;
    static int networkFinishedIndex;
    static int networkDownloadProgressIndex;
    static int replyFinishedIndex;
    static int replyDownloadProgressIndex;

private:
    QQmlEngine *m_engine;
    QQmlFilePrivate *m_p;

    int m_redirectCount;
    QNetworkReply *m_reply;
};
#endif

class QQmlFilePrivate
{
public:
    QQmlFilePrivate();

    mutable QUrl url;
    mutable QString urlString;

    QByteArray data;

    enum Error {
        None, NotFound, CaseMismatch, Network
    };

    Error error;
    QString errorString;
#if QT_CONFIG(qml_network)
    QQmlFileNetworkReply *reply;
#endif
};

#if QT_CONFIG(qml_network)
int QQmlFileNetworkReply::finishedIndex = -1;
int QQmlFileNetworkReply::downloadProgressIndex = -1;
int QQmlFileNetworkReply::networkFinishedIndex = -1;
int QQmlFileNetworkReply::networkDownloadProgressIndex = -1;
int QQmlFileNetworkReply::replyFinishedIndex = -1;
int QQmlFileNetworkReply::replyDownloadProgressIndex = -1;

QQmlFileNetworkReply::QQmlFileNetworkReply(QQmlEngine *e, QQmlFilePrivate *p, const QUrl &url)
: m_engine(e), m_p(p), m_redirectCount(0), m_reply(nullptr)
{
    if (finishedIndex == -1) {
        finishedIndex = QMetaMethod::fromSignal(&QQmlFileNetworkReply::finished).methodIndex();
        downloadProgressIndex = QMetaMethod::fromSignal(&QQmlFileNetworkReply::downloadProgress).methodIndex();
        const QMetaObject *smo = &staticMetaObject;
        networkFinishedIndex = smo->indexOfMethod("networkFinished()");
        networkDownloadProgressIndex = smo->indexOfMethod("networkDownloadProgress(qint64,qint64)");

        replyFinishedIndex = QMetaMethod::fromSignal(&QNetworkReply::finished).methodIndex();
        replyDownloadProgressIndex = QMetaMethod::fromSignal(&QNetworkReply::downloadProgress).methodIndex();
    }
    Q_ASSERT(finishedIndex != -1 && downloadProgressIndex != -1 &&
             networkFinishedIndex != -1 && networkDownloadProgressIndex != -1 &&
             replyFinishedIndex != -1 && replyDownloadProgressIndex != -1);

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    m_reply = m_engine->networkAccessManager()->get(req);
    QMetaObject::connect(m_reply, replyFinishedIndex, this, networkFinishedIndex);
    QMetaObject::connect(m_reply, replyDownloadProgressIndex, this, networkDownloadProgressIndex);
}

QQmlFileNetworkReply::~QQmlFileNetworkReply()
{
    if (m_reply) {
        m_reply->disconnect();
        m_reply->deleteLater();
    }
}

void QQmlFileNetworkReply::networkFinished()
{
    ++m_redirectCount;
    if (m_redirectCount < QQMLFILE_MAX_REDIRECT_RECURSION) {
        QVariant redirect = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = m_reply->url().resolved(redirect.toUrl());

            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

            m_reply->deleteLater();
            m_reply = m_engine->networkAccessManager()->get(req);

            QMetaObject::connect(m_reply, replyFinishedIndex,
                                 this, networkFinishedIndex);
            QMetaObject::connect(m_reply, replyDownloadProgressIndex,
                                 this, networkDownloadProgressIndex);

            return;
        }
    }

    if (m_reply->error()) {
        m_p->errorString = m_reply->errorString();
        m_p->error = QQmlFilePrivate::Network;
    } else {
        m_p->data = m_reply->readAll();
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    m_p->reply = nullptr;
    emit finished();
    delete this;
}

void QQmlFileNetworkReply::networkDownloadProgress(qint64 a, qint64 b)
{
    emit downloadProgress(a, b);
}
#endif // qml_network

QQmlFilePrivate::QQmlFilePrivate()
: error(None)
#if QT_CONFIG(qml_network)
, reply(nullptr)
#endif
{
}

QQmlFile::QQmlFile()
: d(new QQmlFilePrivate)
{
}

QQmlFile::QQmlFile(QQmlEngine *e, const QUrl &url)
: d(new QQmlFilePrivate)
{
    load(e, url);
}

QQmlFile::QQmlFile(QQmlEngine *e, const QString &url)
    : QQmlFile(e, QUrl(url))
{
}

QQmlFile::~QQmlFile()
{
#if QT_CONFIG(qml_network)
    delete d->reply;
#endif
    delete d;
    d = nullptr;
}

bool QQmlFile::isNull() const
{
    return status() == Null;
}

bool QQmlFile::isReady() const
{
    return status() == Ready;
}

bool QQmlFile::isError() const
{
    return status() == Error;
}

bool QQmlFile::isLoading() const
{
    return status() == Loading;
}

QUrl QQmlFile::url() const
{
    if (!d->urlString.isEmpty()) {
        d->url = QUrl(d->urlString);
        d->urlString = QString();
    }
    return d->url;
}

QQmlFile::Status QQmlFile::status() const
{
    if (d->url.isEmpty() && d->urlString.isEmpty())
        return Null;
#if QT_CONFIG(qml_network)
    else if (d->reply)
        return Loading;
#endif
    else if (d->error != QQmlFilePrivate::None)
        return Error;
    else
        return Ready;
}

QString QQmlFile::error() const
{
    switch (d->error) {
    default:
    case QQmlFilePrivate::None:
        return QString();
    case QQmlFilePrivate::NotFound:
        return QLatin1String("File not found");
    case QQmlFilePrivate::CaseMismatch:
        return QLatin1String("File name case mismatch");
    }
}

qint64 QQmlFile::size() const
{
    return d->data.size();
}

const char *QQmlFile::data() const
{
    return d->data.constData();
}

QByteArray QQmlFile::dataByteArray() const
{
    return d->data;
}

void QQmlFile::load(QQmlEngine *engine, const QUrl &url)
{
    Q_ASSERT(engine);

    clear();
    d->url = url;

    if (isLocalFile(url)) {
        QString lf = urlToLocalFileOrQrc(url);

        if (!QQml_isFileCaseCorrect(lf)) {
            d->error = QQmlFilePrivate::CaseMismatch;
            return;
        }

        QFile file(lf);
        if (file.open(QFile::ReadOnly)) {
            d->data = file.readAll();
        } else {
            d->error = QQmlFilePrivate::NotFound;
        }
    } else {
#if QT_CONFIG(qml_network)
        d->reply = new QQmlFileNetworkReply(engine, d, url);
#else
        d->error = QQmlFilePrivate::NotFound;
#endif
    }
}

void QQmlFile::load(QQmlEngine *engine, const QString &url)
{
    Q_ASSERT(engine);

    clear();

    d->urlString = url;

    if (isLocalFile(url)) {
        QString lf = urlToLocalFileOrQrc(url);

        if (!QQml_isFileCaseCorrect(lf)) {
            d->error = QQmlFilePrivate::CaseMismatch;
            return;
        }

        QFile file(lf);
        if (file.open(QFile::ReadOnly)) {
            d->data = file.readAll();
        } else {
            d->error = QQmlFilePrivate::NotFound;
        }
    } else {
#if QT_CONFIG(qml_network)
        QUrl qurl(url);
        d->url = qurl;
        d->urlString = QString();
        d->reply = new QQmlFileNetworkReply(engine, d, qurl);
#else
        d->error = QQmlFilePrivate::NotFound;
#endif
    }
}

void QQmlFile::clear()
{
    d->url = QUrl();
    d->urlString = QString();
    d->data = QByteArray();
    d->error = QQmlFilePrivate::None;
}

void QQmlFile::clear(QObject *)
{
    clear();
}

#if QT_CONFIG(qml_network)
bool QQmlFile::connectFinished(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QQmlFile: connectFinished() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(finished()),
                            object, method);
}

bool QQmlFile::connectFinished(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QQmlFile: connectFinished() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QQmlFileNetworkReply::finishedIndex,
                                object, method);
}

bool QQmlFile::connectDownloadProgress(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QQmlFile: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)),
                            object, method);
}

bool QQmlFile::connectDownloadProgress(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QQmlFile: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QQmlFileNetworkReply::downloadProgressIndex,
                                object, method);
}
#endif

/*!
Returns true if QQmlFile will open \a url synchronously.

Synchronous urls have a qrc:/ or file:// scheme.

\note On Android, urls with assets:/ scheme are also considered synchronous.
*/
bool QQmlFile::isSynchronous(const QUrl &url)
{
    QString scheme = url.scheme();

    if ((scheme.size() == 4 && 0 == scheme.compare(QLatin1String(file_string), Qt::CaseInsensitive)) ||
        (scheme.size() == 3 && 0 == scheme.compare(QLatin1String(qrc_string), Qt::CaseInsensitive))) {
        return true;

#if defined(Q_OS_ANDROID)
    } else if (scheme.length() == 6 && 0 == scheme.compare(QLatin1String(assets_string), Qt::CaseInsensitive)) {
        return true;
    } else if (scheme.length() == 7 && 0 == scheme.compare(QLatin1String(content_string), Qt::CaseInsensitive)) {
        return true;
#endif

    } else {
        return false;
    }
}

/*!
Returns true if QQmlFile will open \a url synchronously.

Synchronous urls have a qrc:/ or file:// scheme.

\note On Android, urls with assets:/ scheme are also considered synchronous.
*/
bool QQmlFile::isSynchronous(const QString &url)
{
    if (url.size() < 5 /* qrc:/ */)
        return false;

    QChar f = url[0];

    if (f == QLatin1Char('f') || f == QLatin1Char('F')) {

        return url.size() >= 7 /* file:// */ &&
               url.startsWith(QLatin1String(file_string), Qt::CaseInsensitive) &&
               url[4] == QLatin1Char(':') && url[5] == QLatin1Char('/') && url[6] == QLatin1Char('/');

    } else if (f == QLatin1Char('q') || f == QLatin1Char('Q')) {

        return url.size() >= 5 /* qrc:/ */ &&
               url.startsWith(QLatin1String(qrc_string), Qt::CaseInsensitive) &&
               url[3] == QLatin1Char(':') && url[4] == QLatin1Char('/');

    }

#if defined(Q_OS_ANDROID)
    else if (f == QLatin1Char('a') || f == QLatin1Char('A')) {
        return url.length() >= 8 /* assets:/ */ &&
               url.startsWith(QLatin1String(assets_string), Qt::CaseInsensitive) &&
               url[6] == QLatin1Char(':') && url[7] == QLatin1Char('/');
    } else if (f == QLatin1Char('c') || f == QLatin1Char('C')) {
        return url.length() >= 9 /* content:/ */ &&
               url.startsWith(QLatin1String(content_string), Qt::CaseInsensitive) &&
               url[7] == QLatin1Char(':') && url[8] == QLatin1Char('/');
    }
#endif

    return false;
}

#if defined(Q_OS_ANDROID)
static bool hasLocalContentAuthority(const QUrl &url)
{
    const QString authority = url.authority();
    return authority.isEmpty()
            || authority == QLatin1String(authority_externalstorage)
            || authority == QLatin1String(authority_downloads_documents)
            || authority == QLatin1String(authority_media_documents);
}
#endif

/*!
Returns true if \a url is a local file that can be opened with QFile.

Local file urls have either a qrc:/ or file:// scheme.

\note On Android, urls with assets:/ scheme are also considered local files.
*/
bool QQmlFile::isLocalFile(const QUrl &url)
{
    QString scheme = url.scheme();

    // file: URLs with two slashes following the scheme can be interpreted as local files
    // where the slashes are part of the path. Therefore, disregard the authority.
    // See QUrl::toLocalFile().
    if (scheme.size() == 4 && scheme.startsWith(QLatin1String(file_string), Qt::CaseInsensitive))
        return true;

    if (scheme.size() == 3 && scheme.startsWith(QLatin1String(qrc_string), Qt::CaseInsensitive))
        return url.authority().isEmpty();

#if defined(Q_OS_ANDROID)
    if (scheme.length() == 6
         && scheme.startsWith(QLatin1String(assets_string), Qt::CaseInsensitive))
        return url.authority().isEmpty();
    if (scheme.length() == 7
         && scheme.startsWith(QLatin1String(content_string), Qt::CaseInsensitive))
        return hasLocalContentAuthority(url);
#endif

    return false;
}

static bool hasScheme(const QString &url, const char *scheme, qsizetype schemeLength)
{
    const qsizetype urlLength = url.size();

    if (urlLength < schemeLength + 1)
        return false;

    if (!url.startsWith(QLatin1String(scheme, scheme + schemeLength), Qt::CaseInsensitive))
        return false;

    if (url[schemeLength] != QLatin1Char(':'))
        return false;

    return true;
}

static qsizetype authorityOffset(const QString &url, qsizetype schemeLength)
{
    const qsizetype urlLength = url.size();

    if (urlLength < schemeLength + 3)
        return -1;

    const QLatin1Char slash('/');
    if (url[schemeLength + 1] == slash && url[schemeLength + 2] == slash) {
        // Exactly two slashes denote an authority.
        if (urlLength < schemeLength + 4 || url[schemeLength + 3] != slash)
            return schemeLength + 3;
    }

    return -1;
}

#if defined(Q_OS_ANDROID)
static bool hasLocalContentAuthority(const QString &url, qsizetype schemeLength)
{
    const qsizetype offset = authorityOffset(url, schemeLength);
    if (offset == -1)
        return true; // no authority is a local authority.

    const QString authorityAndPath = url.sliced(offset);
    return authorityAndPath.startsWith(QLatin1String(authority_externalstorage))
         || authorityAndPath.startsWith(QLatin1String(authority_downloads_documents))
         || authorityAndPath.startsWith(QLatin1String(authority_media_documents));
}

#endif

/*!
Returns true if \a url is a local file that can be opened with QFile.

Local file urls have either a qrc: or file: scheme.

\note On Android, urls with assets: or content: scheme are also considered local files.
*/
bool QQmlFile::isLocalFile(const QString &url)
{
    if (url.size() < 4 /* qrc: */)
        return false;

    switch (url[0].toLatin1()) {
    case 'f':
    case 'F': {
        // file: URLs with two slashes following the scheme can be interpreted as local files
        // where the slashes are part of the path. Therefore, disregard the authority.
        // See QUrl::toLocalFile().
        const qsizetype fileLength = strlen(file_string);
        return url.startsWith(QLatin1String(file_string, file_string + fileLength),
                              Qt::CaseInsensitive)
                && url.size() > fileLength
                && url[fileLength] == QLatin1Char(':');
    }
    case 'q':
    case 'Q':
        return hasScheme(url, qrc_string, strlen(qrc_string))
            && authorityOffset(url, strlen(qrc_string)) == -1;
#if defined(Q_OS_ANDROID)
    case 'a':
    case 'A':
        return hasScheme(url, assets_string, strlen(assets_string))
            && authorityOffset(url, strlen(assets_string)) == -1;
    case 'c':
    case 'C':
        return hasScheme(url, content_string, strlen(content_string))
            && hasLocalContentAuthority(url, strlen(content_string));
#endif
    default:
        break;
    }

    return false;
}

/*!
If \a url is a local file returns a path suitable for passing to QFile.  Otherwise returns an
empty string.
*/
QString QQmlFile::urlToLocalFileOrQrc(const QUrl& url)
{
    if (url.scheme().compare(QLatin1String("qrc"), Qt::CaseInsensitive) == 0) {
        if (url.authority().isEmpty())
            return QLatin1Char(':') + url.path();
        return QString();
    }

#if defined(Q_OS_ANDROID)
    if (url.scheme().compare(QLatin1String("assets"), Qt::CaseInsensitive) == 0)
        return url.authority().isEmpty() ? url.toString() : QString();
    if (url.scheme().compare(QLatin1String("content"), Qt::CaseInsensitive) == 0) {
        if (hasLocalContentAuthority(url))
            return url.toString();
        return QString();
    }
#endif
    return url.toLocalFile();
}

static QString toLocalFile(const QString &url)
{
    const QUrl file(url);
    if (!file.isLocalFile())
        return QString();

    // QUrl::toLocalFile() interprets two slashes as part of the path.
    // Therefore windows hostnames like "//servername/path/to/file.txt" are preserved.

    return file.toLocalFile();
}

static bool isDoubleSlashed(const QString &url, qsizetype offset)
{
    const qsizetype urlLength = url.size();
    if (urlLength < offset + 2)
        return false;

    const QLatin1Char slash('/');
    if (url[offset] != slash || url[offset + 1] != slash)
        return false;

    if (urlLength < offset + 3)
        return true;

    return url[offset + 2] != slash;
}

/*!
If \a url is a local file returns a path suitable for passing to QFile.  Otherwise returns an
empty string.
*/
QString QQmlFile::urlToLocalFileOrQrc(const QString& url)
{
    if (url.startsWith(QLatin1String("qrc://"), Qt::CaseInsensitive)) {
        // Exactly two slashes are bad because that's a URL authority.
        // One slash is fine and >= 3 slashes are file.
        if (url.size() == 6 || url[6] != QLatin1Char('/')) {
            Q_ASSERT(isDoubleSlashed(url, strlen("qrc:")));
            return QString();
        }
        Q_ASSERT(!isDoubleSlashed(url, strlen("qrc:")));
        return QLatin1Char(':') + QStringView{url}.mid(6);
    }

    if (url.startsWith(QLatin1String("qrc:"), Qt::CaseInsensitive)) {
        Q_ASSERT(!isDoubleSlashed(url, strlen("qrc:")));
        if (url.size() > 4)
            return QLatin1Char(':') + QStringView{url}.mid(4);
        return QStringLiteral(":");
    }

#if defined(Q_OS_ANDROID)
    if (url.startsWith(QLatin1String("assets:"), Qt::CaseInsensitive))
        return isDoubleSlashed(url, strlen("assets:")) ? QString() : url;
    if (hasScheme(url, content_string, strlen(content_string)))
        return hasLocalContentAuthority(url, strlen(content_string)) ? url : QString();
#endif

    return toLocalFile(url);
}

QT_END_NAMESPACE

#include "qqmlfile.moc"
