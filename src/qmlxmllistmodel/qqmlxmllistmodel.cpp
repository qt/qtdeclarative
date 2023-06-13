// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlxmllistmodel_p.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qfuturewatcher.h>
#include <QtCore/qtimer.h>
#include <QtCore/qxmlstream.h>

#if QT_CONFIG(qml_network)
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qnetworkrequest.h>
#endif

Q_DECLARE_METATYPE(QQmlXmlListModelQueryResult)

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQml.XmlListModel
    \title Qt XmlListModel QML Types
    \keyword Qt XmlListModel QML Types
    \ingroup qmlmodules
    \brief Provides QML types for creating models from XML data

    This QML module contains types for creating models from XML data.

    To use the types in this module, import the module with the following line:

    \qml
    import QtQml.XmlListModel
    \endqml
*/

/*!
    \qmltype XmlListModelRole
    \inqmlmodule QtQml.XmlListModel
    \brief For specifying a role to an \l XmlListModel.

    \sa {All QML Types}{Qt QML}
*/

/*!
    \qmlproperty string QtQml.XmlListModel::XmlListModelRole::name

    The name for the role. This name is used to access the model data for this
    role.

    For example, the following model has a role named "title", which can be
    accessed from the view's delegate:

    \qml
    XmlListModel {
        id: xmlModel
        source: "file.xml"
        query: "/documents/document"
        XmlListModelRole { name: "title"; elementName: "title" }
    }
    \endqml

    \qml
    ListView {
        model: xmlModel
        delegate: Text { text: title }
    }
    \endqml
*/
QString QQmlXmlListModelRole::name() const
{
    return m_name;
}

void QQmlXmlListModelRole::setName(const QString &name)
{
    if (name == m_name)
        return;
    m_name = name;
    Q_EMIT nameChanged();
}

/*!
    \qmlproperty string QtQml.XmlListModel::XmlListModelRole::elementName

    The name of the XML element, or a path to the XML element, that will be
    used to read the data. The element must actually contain text.

    Optionally the \l attributeName property can be specified to extract
    the data.

//! [basic-example]
    For example, the following model has a role named "title", which reads the
    data from the XML element \c {<title>}. It also has another role named
    "timestamp", which uses the same XML element \c {<title>}, but reads its
    "created" attribute to extract the actual value.

    \qml
    XmlListModel {
        id: xmlModel
        source: "file.xml"
        query: "/documents/document"
        XmlListModelRole { name: "title"; elementName: "title" }
        XmlListModelRole {
            name: "timestamp"
            elementName: "title"
            attributeName: "created"
        }
    }

    ListView {
        anchors.fill: parent
        model: xmlModel
        delegate: Text { text: title + " created on " + timestamp }
    }
    \endqml
//! [basic-example]

//! [empty-elementName-example]
    When the \l attributeName is specified, the \l elementName can be left
    empty. In this case the attribute of the top level XML element of the query
    will be read.

    For example, if you have the following xml document:

    \code
    <documents>
        <document title="Title1"/>
        <document title="Title2"/>
    </documents>
    \endcode

    To extract the document titles you need the following model:

    \qml
    XmlListModel {
        id: xmlModel
        source: "file.xml"
        query: "/documents/document"
        XmlListModelRole {
            name: "title"
            elementName: ""
            attributeName: "title"
        }
    }
    \endqml
//! [empty-elementName-example]

    The elementName property can actually contain a path to the nested xml
    element. All the elements in the path must be joined with the \c {'/'}
    character.

    For example, if you have the following xml document:
    \code
    <documents>
        <document>
            <title>Title1</title>
            <info>
                <num_pages>10</num_pages>
            </info>
        </document>
        <document>
            <title>Title2</title>
            <info>
                <num_pages>20</num_pages>
            </info>
        </document>
    </documents>
    \endcode

    You can extract the number of pages with the following role:

    \qml
    XmlListModel {
        id: xmlModel
        source: "file.xml"
        query: "/documents/document"
        // ...
        XmlListModelRole {
            name: "pages"
            elementName: "info/num_pages"
        }
    }
    \endqml

    \note The path to the element must not start or end with \c {'/'}.

    \sa attributeName
*/
QString QQmlXmlListModelRole::elementName() const
{
    return m_elementName;
}

void QQmlXmlListModelRole::setElementName(const QString &name)
{
    if (name.startsWith(QLatin1Char('/'))) {
        qmlWarning(this) << tr("An XML element must not start with '/'");
        return;
    } else if (name.endsWith(QLatin1Char('/'))) {
        qmlWarning(this) << tr("An XML element must not end with '/'");
        return;
    } else if (name.contains(QStringLiteral("//"))) {
        qmlWarning(this) << tr("An XML element must not contain \"//\"");
        return;
    }

    if (name == m_elementName)
        return;
    m_elementName = name;
    Q_EMIT elementNameChanged();
}

/*!
    \qmlproperty string QtQml.XmlListModel::XmlListModelRole::attributeName

    The attribute of the XML element that will be used to read the data.
    The XML element is specified by \l elementName property.

    \include qqmlxmllistmodel.cpp basic-example

    \include qqmlxmllistmodel.cpp empty-elementName-example

    If you do not need to parse any attributes for the specified XML element,
    simply leave this property blank.

    \sa elementName
*/
QString QQmlXmlListModelRole::attributeName() const
{
    return m_attributeName;
}

void QQmlXmlListModelRole::setAttributeName(const QString &attributeName)
{
    if (m_attributeName == attributeName)
        return;
    m_attributeName = attributeName;
    Q_EMIT attributeNameChanged();
}

bool QQmlXmlListModelRole::isValid() const
{
    return !m_name.isEmpty();
}

/*!
    \qmltype XmlListModel
    \inqmlmodule QtQml.XmlListModel
    \brief For specifying a read-only model using XML data.

    To use this element, you will need to import the module with the following line:
    \code
    import QtQml.XmlListModel
    \endcode

    XmlListModel is used to create a read-only model from XML data. It can be
    used as a data source for view elements (such as ListView, PathView,
    GridView) and other elements that interact with model data (such as
    Repeater).

    \note This model \b {does not} support the XPath queries. It supports simple
    slash-separated paths and, optionally, one attribute for each element.

    For example, if there is an XML document at https://www.qt.io/blog/rss.xml
    like this:

    \code
    <?xml version="1.0" encoding="UTF-8"?>
    <rss version="2.0">
      ...
      <channel>
        <item>
          <title>Qt 6.0.2 Released</title>
          <link>https://www.qt.io/blog/qt-6.0.2-released</link>
          <pubDate>Wed, 03 Mar 2021 12:40:43 GMT</pubDate>
        </item>
        <item>
          <title>Qt 6.1 Beta Released</title>
          <link>https://www.qt.io/blog/qt-6.1-beta-released</link>
          <pubDate>Tue, 02 Mar 2021 13:05:47 GMT</pubDate>
        </item>
        <item>
          <title>Qt Creator 4.14.1 released</title>
          <link>https://www.qt.io/blog/qt-creator-4.14.1-released</link>
          <pubDate>Wed, 24 Feb 2021 13:53:21 GMT</pubDate>
        </item>
      </channel>
    </rss>
    \endcode

    A XmlListModel could create a model from this data, like this:

    \qml
    import QtQml.XmlListModel

    XmlListModel {
        id: xmlModel
        source: "https://www.qt.io/blog/rss.xml"
        query: "/rss/channel/item"

        XmlListModelRole { name: "title"; elementName: "title" }
        XmlListModelRole { name: "pubDate"; elementName: "pubDate" }
        XmlListModelRole { name: "link"; elementName: "link" }
    }
    \endqml

    The \l {XmlListModel::query}{query} value of "/rss/channel/item" specifies
    that the XmlListModel should generate a model item for each \c {<item>} in
    the XML document.

    The \l [QML] {XmlListModelRole} objects define the model item attributes.
    Here, each model item will have \c title, \c pubDate and \c link attributes
    that match the \c title, \c pubDate and \c link values of its corresponding
    \c {<item>}.
    (See \l [QML] {XmlListModelRole} documentation for more examples.)

    The model could be used in a ListView, like this:

    \qml
    ListView {
        width: 180; height: 300
        model: xmlModel
        delegate: Text { text: title + ": " + pubDate + "; link: " + link }
    }
    \endqml

    The \l XmlListModel data is loaded asynchronously, and \l status
    is set to \c XmlListModel.Ready when loading is complete.
    Note this means when \l XmlListModel is used for a view, the view is not
    populated until the model is loaded.
*/

QQmlXmlListModel::QQmlXmlListModel(QObject *parent) : QAbstractListModel(parent) { }

QQmlXmlListModel::~QQmlXmlListModel()
{
    // Cancel all objects
    for (auto &w : m_watchers.values())
        w->cancel();
    // Wait until all objects are finished
    while (!m_watchers.isEmpty()) {
        auto it = m_watchers.begin();
        it.value()->waitForFinished();
        // Explicitly delete the watcher here, because the connected lambda
        // would not be called until processEvents() is called
        delete it.value();
        m_watchers.erase(it);
    }
}

QModelIndex QQmlXmlListModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && m_size ? createIndex(row, column)
                                                                  : QModelIndex();
}

int QQmlXmlListModel::rowCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? m_size : 0;
}

QVariant QQmlXmlListModel::data(const QModelIndex &index, int role) const
{
    const int roleIndex = m_roles.indexOf(role);
    return (roleIndex == -1 || !index.isValid()) ? QVariant()
                                                 : m_data.value(index.row()).value(roleIndex);
}

QHash<int, QByteArray> QQmlXmlListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    for (int i = 0; i < m_roles.size(); ++i)
        roleNames.insert(m_roles.at(i), m_roleNames.at(i).toUtf8());
    return roleNames;
}

/*!
    \qmlproperty int QtQml.XmlListModel::XmlListModel::count
    The number of data entries in the model.
*/
int QQmlXmlListModel::count() const
{
    return m_size;
}

/*!
    \qmlproperty url QtQml.XmlListModel::XmlListModel::source
    The location of the XML data source.
*/
QUrl QQmlXmlListModel::source() const
{
    return m_source;
}

void QQmlXmlListModel::setSource(const QUrl &src)
{
    if (m_source != src) {
        m_source = src;
        reload();
        Q_EMIT sourceChanged();
    }
}

/*!
    \qmlproperty string QtQml.XmlListModel::XmlListModel::query
    A string representing the base path for creating model items from this
    model's \l [QML] {XmlListModelRole} objects. The query should start with
    \c {'/'}.
*/
QString QQmlXmlListModel::query() const
{
    return m_query;
}

void QQmlXmlListModel::setQuery(const QString &query)
{
    if (!query.startsWith(QLatin1Char('/'))) {
        qmlWarning(this) << QCoreApplication::translate(
                "XmlListModelRoleList", "An XmlListModel query must start with '/'");
        return;
    }

    if (m_query != query) {
        m_query = query;
        reload();
        Q_EMIT queryChanged();
    }
}

/*!
    \qmlproperty list<XmlListModelRole> QtQml.XmlListModel::XmlListModel::roles

    The roles to make available for this model.
*/
QQmlListProperty<QQmlXmlListModelRole> QQmlXmlListModel::roleObjects()
{
    QQmlListProperty<QQmlXmlListModelRole> list(this, &m_roleObjects);
    list.append = &QQmlXmlListModel::appendRole;
    list.clear = &QQmlXmlListModel::clearRole;
    return list;
}

void QQmlXmlListModel::appendRole(QQmlXmlListModelRole *role)
{
    if (role) {
        int i = m_roleObjects.size();
        m_roleObjects.append(role);
        if (m_roleNames.contains(role->name())) {
            qmlWarning(role)
                    << QQmlXmlListModel::tr(
                               "\"%1\" duplicates a previous role name and will be disabled.")
                               .arg(role->name());
            return;
        }
        m_roles.insert(i, m_highestRole);
        m_roleNames.insert(i, role->name());
        ++m_highestRole;
    }
}

void QQmlXmlListModel::clearRole()
{
    m_roles.clear();
    m_roleNames.clear();
    m_roleObjects.clear();
}

void QQmlXmlListModel::appendRole(QQmlListProperty<QQmlXmlListModelRole> *list,
                                  QQmlXmlListModelRole *role)
{
    auto object = qobject_cast<QQmlXmlListModel *>(list->object);
    if (object) // role is checked inside appendRole
        object->appendRole(role);
}

void QQmlXmlListModel::clearRole(QQmlListProperty<QQmlXmlListModelRole> *list)
{
    auto object = qobject_cast<QQmlXmlListModel *>(list->object);
    if (object)
        object->clearRole();
}

void QQmlXmlListModel::tryExecuteQuery(const QByteArray &data)
{
    auto job = createJob(data);
    m_queryId = job.queryId;
    QQmlXmlListModelQueryRunnable *runnable = new QQmlXmlListModelQueryRunnable(std::move(job));
    if (runnable) {
        auto future = runnable->future();
        auto *watcher = new ResultFutureWatcher();
        // No need to connect to canceled signal, because it just notifies that
        // QFuture::cancel() was called. We will get the finished() signal in
        // both cases.
        connect(watcher, &ResultFutureWatcher::finished, this, [id = m_queryId, this]() {
            auto *watcher = static_cast<ResultFutureWatcher *>(sender());
            if (watcher) {
                if (!watcher->isCanceled()) {
                    QQmlXmlListModelQueryResult result = watcher->result();
                    // handle errors
                    for (const auto &errorInfo : result.errors)
                        queryError(errorInfo.first, errorInfo.second);
                    // fill results
                    queryCompleted(result);
                }
                // remove from watchers
                m_watchers.remove(id);
                watcher->deleteLater();
            }
        });
        m_watchers[m_queryId] = watcher;
        watcher->setFuture(future);
        QThreadPool::globalInstance()->start(runnable);
    } else {
        m_errorString = tr("Failed to create an instance of QRunnable query object");
        m_status = QQmlXmlListModel::Error;
        m_queryId = -1;
        Q_EMIT statusChanged(m_status);
    }
}

QQmlXmlListModelQueryJob QQmlXmlListModel::createJob(const QByteArray &data)
{
    QQmlXmlListModelQueryJob job;
    job.queryId = nextQueryId();
    job.data = data;
    job.query = m_query;

    for (int i = 0; i < m_roleObjects.size(); i++) {
        if (!m_roleObjects.at(i)->isValid()) {
            job.roleNames << QString();
            job.elementNames << QString();
            job.elementAttributes << QString();
            continue;
        }
        job.roleNames << m_roleObjects.at(i)->name();
        job.elementNames << m_roleObjects.at(i)->elementName();
        job.elementAttributes << m_roleObjects.at(i)->attributeName();
        job.roleQueryErrorId << static_cast<void *>(m_roleObjects.at(i));
    }

    return job;
}

int QQmlXmlListModel::nextQueryId()
{
    m_nextQueryIdGenerator++;
    if (m_nextQueryIdGenerator <= 0)
        m_nextQueryIdGenerator = 1;
    return m_nextQueryIdGenerator;
}

/*!
    \qmlproperty enumeration QtQml.XmlListModel::XmlListModel::status
    Specifies the model loading status, which can be one of the following:

    \value XmlListModel.Null    No XML data has been set for this model.
    \value XmlListModel.Ready   The XML data has been loaded into the model.
    \value XmlListModel.Loading The model is in the process of reading and
                                loading XML data.
    \value XmlListModel.Error   An error occurred while the model was loading. See
                                \l errorString() for details about the error.

    \sa progress
*/
QQmlXmlListModel::Status QQmlXmlListModel::status() const
{
    return m_status;
}

/*!
    \qmlproperty real QtQml.XmlListModel::XmlListModel::progress

    This indicates the current progress of the downloading of the XML data
    source. This value ranges from 0.0 (no data downloaded) to
    1.0 (all data downloaded). If the XML data is not from a remote source,
    the progress becomes 1.0 as soon as the data is read.

    Note that when the progress is 1.0, the XML data has been downloaded, but
    it is yet to be loaded into the model at this point. Use the status
    property to find out when the XML data has been read and loaded into
    the model.

    \sa status, source
*/
qreal QQmlXmlListModel::progress() const
{
    return m_progress;
}

/*!
    \qmlmethod QtQml.XmlListModel::XmlListModel::errorString()

    Returns a string description of the last error that occurred
    if \l status is \l {XmlListModel}.Error.
*/
QString QQmlXmlListModel::errorString() const
{
    return m_errorString;
}

void QQmlXmlListModel::classBegin()
{
    m_isComponentComplete = false;
}

void QQmlXmlListModel::componentComplete()
{
    m_isComponentComplete = true;
    reload();
}

/*!
    \qmlmethod QtQml.XmlListModel::XmlListModel::reload()

    Reloads the model.
*/
void QQmlXmlListModel::reload()
{
    if (!m_isComponentComplete)
        return;

    if (m_queryId > 0 && m_watchers.contains(m_queryId))
        m_watchers[m_queryId]->cancel();

    m_queryId = -1;

    if (m_size < 0)
        m_size = 0;

#if QT_CONFIG(qml_network)
    if (m_reply) {
        m_reply->abort();
        deleteReply();
    }
#endif

    const QQmlContext *context = qmlContext(this);
    const auto resolvedSource = context ? context->resolvedUrl(m_source) : m_source;

    if (resolvedSource.isEmpty()) {
        m_queryId = 0;
        notifyQueryStarted(false);
        QTimer::singleShot(0, this, &QQmlXmlListModel::dataCleared);
    } else if (QQmlFile::isLocalFile(resolvedSource)) {
        QFile file(QQmlFile::urlToLocalFileOrQrc(resolvedSource));
        const bool opened = file.open(QIODevice::ReadOnly);
        if (!opened)
            qWarning("Failed to open file %s: %s", qPrintable(file.fileName()),
                     qPrintable(file.errorString()));
        QByteArray data = opened ? file.readAll() : QByteArray();
        notifyQueryStarted(false);
        if (data.isEmpty()) {
            m_queryId = 0;
            QTimer::singleShot(0, this, &QQmlXmlListModel::dataCleared);
        } else {
            tryExecuteQuery(data);
        }
    } else {
#if QT_CONFIG(qml_network)
        notifyQueryStarted(true);
        QNetworkRequest req(resolvedSource);
        req.setRawHeader("Accept", "application/xml,*/*");
        m_reply = qmlContext(this)->engine()->networkAccessManager()->get(req);

        QObject::connect(m_reply, &QNetworkReply::finished, this,
                         &QQmlXmlListModel::requestFinished);
        QObject::connect(m_reply, &QNetworkReply::downloadProgress, this,
                         &QQmlXmlListModel::requestProgress);
#else
        m_queryId = 0;
        notifyQueryStarted(false);
        QTimer::singleShot(0, this, &QQmlXmlListModel::dataCleared);
#endif
    }
}

#define XMLLISTMODEL_MAX_REDIRECT 16

#if QT_CONFIG(qml_network)
void QQmlXmlListModel::requestFinished()
{
    m_redirectCount++;
    if (m_redirectCount < XMLLISTMODEL_MAX_REDIRECT) {
        QVariant redirect = m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = m_reply->url().resolved(redirect.toUrl());
            deleteReply();
            setSource(url);
            return;
        }
    }
    m_redirectCount = 0;

    if (m_reply->error() != QNetworkReply::NoError) {
        m_errorString = m_reply->errorString();
        deleteReply();

        if (m_size > 0) {
            beginRemoveRows(QModelIndex(), 0, m_size - 1);
            m_data.clear();
            m_size = 0;
            endRemoveRows();
            Q_EMIT countChanged();
        }

        m_status = Error;
        m_queryId = -1;
        Q_EMIT statusChanged(m_status);
    } else {
        QByteArray data = m_reply->readAll();
        if (data.isEmpty()) {
            m_queryId = 0;
            QTimer::singleShot(0, this, &QQmlXmlListModel::dataCleared);
        } else {
            tryExecuteQuery(data);
        }
        deleteReply();

        m_progress = 1.0;
        Q_EMIT progressChanged(m_progress);
    }
}

void QQmlXmlListModel::deleteReply()
{
    if (m_reply) {
        QObject::disconnect(m_reply, 0, this, 0);
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}
#endif

void QQmlXmlListModel::requestProgress(qint64 received, qint64 total)
{
    if (m_status == Loading && total > 0) {
        m_progress = qreal(received) / total;
        Q_EMIT progressChanged(m_progress);
    }
}

void QQmlXmlListModel::dataCleared()
{
    QQmlXmlListModelQueryResult r;
    r.queryId = 0;
    queryCompleted(r);
}

void QQmlXmlListModel::queryError(void *object, const QString &error)
{
    for (int i = 0; i < m_roleObjects.size(); i++) {
        if (m_roleObjects.at(i) == static_cast<QQmlXmlListModelRole *>(object)) {
            qmlWarning(m_roleObjects.at(i))
                    << QQmlXmlListModel::tr("Query error: \"%1\"").arg(error);
            return;
        }
    }
    qmlWarning(this) << QQmlXmlListModel::tr("Query error: \"%1\"").arg(error);
}

void QQmlXmlListModel::queryCompleted(const QQmlXmlListModelQueryResult &result)
{
    if (result.queryId != m_queryId)
        return;

    int origCount = m_size;
    bool sizeChanged = result.data.size() != m_size;

    if (m_source.isEmpty())
        m_status = Null;
    else
        m_status = Ready;
    m_errorString.clear();
    m_queryId = -1;

    if (origCount > 0) {
        beginRemoveRows(QModelIndex(), 0, origCount - 1);
        endRemoveRows();
    }
    m_size = result.data.size();
    m_data = result.data;

    if (m_size > 0) {
        beginInsertRows(QModelIndex(), 0, m_size - 1);
        endInsertRows();
    }

    if (sizeChanged)
        Q_EMIT countChanged();

    Q_EMIT statusChanged(m_status);
}

void QQmlXmlListModel::notifyQueryStarted(bool remoteSource)
{
    m_progress = remoteSource ? 0.0 : 1.0;
    m_status = QQmlXmlListModel::Loading;
    m_errorString.clear();
    Q_EMIT progressChanged(m_progress);
    Q_EMIT statusChanged(m_status);
}

static qsizetype findIndexOfName(const QStringList &elementNames, const QStringView &name,
                                 qsizetype startIndex = 0)
{
    for (auto idx = startIndex; idx < elementNames.size(); ++idx) {
        if (elementNames[idx].startsWith(name))
            return idx;
    }
    return -1;
}

QQmlXmlListModelQueryRunnable::QQmlXmlListModelQueryRunnable(QQmlXmlListModelQueryJob &&job)
    : m_job(std::move(job))
{
    setAutoDelete(true);
}

void QQmlXmlListModelQueryRunnable::run()
{
    m_promise.start();
    if (!m_promise.isCanceled()) {
        QQmlXmlListModelQueryResult result;
        result.queryId = m_job.queryId;
        doQueryJob(&result);
        m_promise.addResult(std::move(result));
    }
    m_promise.finish();
}

QFuture<QQmlXmlListModelQueryResult> QQmlXmlListModelQueryRunnable::future() const
{
    return m_promise.future();
}

void QQmlXmlListModelQueryRunnable::doQueryJob(QQmlXmlListModelQueryResult *currentResult)
{
    Q_ASSERT(m_job.queryId != -1);

    QByteArray data(m_job.data);
    QXmlStreamReader reader;
    reader.addData(data);

    QStringList items = m_job.query.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    while (!reader.atEnd() && !m_promise.isCanceled()) {
        int i = 0;
        while (i < items.size()) {
            if (reader.readNextStartElement()) {
                if (reader.name() == items.at(i)) {
                    if (i != items.size() - 1) {
                        i++;
                        continue;
                    } else {
                        processElement(currentResult, items.at(i), reader);
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
            if (reader.tokenType() == QXmlStreamReader::Invalid) {
                reader.readNext();
                break;
            } else if (reader.hasError()) {
                reader.raiseError();
                break;
            }
        }
    }
}

void QQmlXmlListModelQueryRunnable::processElement(QQmlXmlListModelQueryResult *currentResult,
                                                   const QString &element, QXmlStreamReader &reader)
{
    if (!reader.isStartElement() || reader.name() != element)
        return;

    const QStringList &elementNames = m_job.elementNames;
    const QStringList &attributes = m_job.elementAttributes;
    QFlatMap<int, QString> results;

    // First of all check all the empty element names. They might have
    // attributes to be read from the current element
    if (!reader.attributes().isEmpty()) {
        for (auto index = 0; index < elementNames.size(); ++index) {
            if (elementNames.at(index).isEmpty() && !attributes.at(index).isEmpty()) {
                const QString &attribute = attributes.at(index);
                if (reader.attributes().hasAttribute(attribute))
                    results[index] = reader.attributes().value(attribute).toString();
            }
        }
    }

    // After that we recursively search for the elements, considering that we
    // can have nested element names in our model, and that the same element
    // can be used multiple types (with different attributes, for example)
    readSubTree(QString(), reader, results, &currentResult->errors);

    if (reader.hasError())
        currentResult->errors.push_back(qMakePair(this, reader.errorString()));

    currentResult->data << results;
}

void QQmlXmlListModelQueryRunnable::readSubTree(const QString &prefix, QXmlStreamReader &reader,
                                                QFlatMap<int, QString> &results,
                                                QList<QPair<void *, QString>> *errors)
{
    const QStringList &elementNames = m_job.elementNames;
    const QStringList &attributes = m_job.elementAttributes;
    while (reader.readNextStartElement()) {
        const auto name = reader.name();
        const QString fullName =
                prefix.isEmpty() ? name.toString() : (prefix + QLatin1Char('/') + name.toString());
        qsizetype index = name.isEmpty() ? -1 : findIndexOfName(elementNames, fullName);
        if (index >= 0) {
            // We can have multiple roles with the same element name, but
            // different attributes, so we need to cache the attributes and
            // element text.
            const auto elementAttributes = reader.attributes();
            // We can read text only when the element actually contains it,
            // otherwise it will be an error. It can also be used to check that
            // we've reached the bottom level.
            QString elementText;
            bool elementTextRead = false;
            while (index >= 0) {
                // if the path matches completely, not just starts with, we
                // need to actually extract value
                if (elementNames[index] == fullName) {
                    QString roleResult;
                    const QString &attribute = attributes.at(index);
                    if (!attribute.isEmpty()) {
                        if (elementAttributes.hasAttribute(attribute)) {
                            roleResult = elementAttributes.value(attributes.at(index)).toString();
                        } else {
                            errors->push_back(qMakePair(m_job.roleQueryErrorId.at(index),
                                                        QLatin1String("Attribute %1 not found")
                                                                .arg(attributes[index])));
                        }
                    } else if (!elementNames.at(index).isEmpty()) {
                        if (!elementTextRead) {
                            elementText =
                                    reader.readElementText(QXmlStreamReader::IncludeChildElements);
                            elementTextRead = true;
                        }
                        roleResult = elementText;
                    }
                    results[index] = roleResult;
                }
                // search for the next role with the same element name
                index = findIndexOfName(elementNames, fullName, index + 1);
            }
            if (!elementTextRead)
                readSubTree(fullName, reader, results, errors);
        } else {
            reader.skipCurrentElement();
        }
    }
}

QT_END_NAMESPACE

#include "moc_qqmlxmllistmodel_p.cpp"
