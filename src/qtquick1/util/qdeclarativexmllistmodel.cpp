/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativexmllistmodel_p.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>

#include <QDebug>
#include <QStringList>
#include <QMap>
#include <QApplication>
#include <QThread>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QXmlNodeModelIndex>
#include <QBuffer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QMutex>

#include <private/qobject_p.h>

Q_DECLARE_METATYPE(QDeclarative1XmlQueryResult)

QT_BEGIN_NAMESPACE




typedef QPair<int, int> QDeclarative1XmlListRange;

#define XMLLISTMODEL_CLEAR_ID 0

/*!
    \qmlclass XmlRole QDeclarative1XmlListModelRole
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
  \since QtQuick 1.0
    \brief The XmlRole element allows you to specify a role for an XmlListModel.

    \sa {QtDeclarative}
*/

/*!
    \qmlproperty string QtQuick1::XmlRole::name

    The name for the role. This name is used to access the model data for this role.

    For example, the following model has a role named "title", which can be accessed
    from the view's delegate:

    \qml
    XmlListModel {
        id: xmlModel
        // ...
        XmlRole {
            name: "title"
            query: "title/string()"
        }
    }
    \endqml

    \qml
    ListView {
        model: xmlModel
        delegate: Text { text: title }
    }
    \endqml
*/

/*!
    \qmlproperty string QtQuick1::XmlRole::query
    The relative XPath expression query for this role. The query must be relative; it cannot start
    with a '/'.

    For example, if there is an XML document like this:

    \quotefile doc/src/snippets/qtquick1/xmlrole.xml
        
    Here are some valid XPath expressions for XmlRole queries on this document:

    \snippet doc/src/snippets/qtquick1/xmlrole.qml 0
    \dots 4
    \snippet doc/src/snippets/qtquick1/xmlrole.qml 1

    See the \l{http://www.w3.org/TR/xpath20/}{W3C XPath 2.0 specification} for more information.
*/

/*!
    \qmlproperty bool QtQuick1::XmlRole::isKey
    Defines whether this is a key role.
    
    Key roles are used to to determine whether a set of values should
    be updated or added to the XML list model when XmlListModel::reload()
    is called.

    \sa XmlListModel
*/

struct XmlQueryJob
{
    int queryId;
    QByteArray data;
    QString query;
    QString namespaces;
    QStringList roleQueries;
    QList<void*> roleQueryErrorId; // the ptr to send back if there is an error
    QStringList keyRoleQueries;
    QStringList keyRoleResultsCache;
    QString prefix;
};

class QDeclarative1XmlQuery : public QObject
{
    Q_OBJECT
public:
    QDeclarative1XmlQuery(QObject *parent=0)
        : QObject(parent), m_queryIds(XMLLISTMODEL_CLEAR_ID + 1) {
        qRegisterMetaType<QDeclarative1XmlQueryResult>("QDeclarative1XmlQueryResult");
        moveToThread(&m_thread);
        m_thread.start(QThread::IdlePriority);
    }

    ~QDeclarative1XmlQuery() {
        if(m_thread.isRunning()) {
            m_thread.quit();
            m_thread.wait();
        }
    }

    void abort(int id) {
        QMutexLocker ml(&m_mutex);
        if (id != -1) {
            m_jobs.remove(id);
        }
    }

    int doQuery(QString query, QString namespaces, QByteArray data, QList<QDeclarative1XmlListModelRole *>* roleObjects, QStringList keyRoleResultsCache) {
        {
            QMutexLocker m1(&m_mutex);
            m_queryIds.ref();
            if (m_queryIds.load() <= 0)
                m_queryIds.store(1);
        }

        XmlQueryJob job;
        job.queryId = m_queryIds.load();
        job.data = data;
        job.query = QLatin1String("doc($src)") + query;
        job.namespaces = namespaces;
        job.keyRoleResultsCache = keyRoleResultsCache;

        for (int i=0; i<roleObjects->count(); i++) {
            if (!roleObjects->at(i)->isValid()) {
                job.roleQueries << QString();
                continue;
            }
            job.roleQueries << roleObjects->at(i)->query();
            job.roleQueryErrorId << static_cast<void*>(roleObjects->at(i));
            if (roleObjects->at(i)->isKey())
                job.keyRoleQueries << job.roleQueries.last();
        }

        {
            QMutexLocker ml(&m_mutex);
            m_jobs.insert(m_queryIds.load(), job);
        }

        QMetaObject::invokeMethod(this, "processQuery", Qt::QueuedConnection, Q_ARG(int, job.queryId));
        return job.queryId;
    }

private slots:
    void processQuery(int queryId) {
        XmlQueryJob job;

        {
            QMutexLocker ml(&m_mutex);
            if (!m_jobs.contains(queryId))
                return;
            job = m_jobs.value(queryId);
        }

        QDeclarative1XmlQueryResult result;
        result.queryId = job.queryId;
        doQueryJob(&job, &result);
        doSubQueryJob(&job, &result);

        {
            QMutexLocker ml(&m_mutex);
            if (m_jobs.contains(queryId)) {
                emit queryCompleted(result);
                m_jobs.remove(queryId);
            }
        }
    }

Q_SIGNALS:
    void queryCompleted(const QDeclarative1XmlQueryResult &);
    void error(void*, const QString&);

protected:


private:
    void doQueryJob(XmlQueryJob *job, QDeclarative1XmlQueryResult *currentResult);
    void doSubQueryJob(XmlQueryJob *job, QDeclarative1XmlQueryResult *currentResult);
    void getValuesOfKeyRoles(const XmlQueryJob& currentJob, QStringList *values, QXmlQuery *query) const;
    void addIndexToRangeList(QList<QDeclarative1XmlListRange> *ranges, int index) const;

private:
    QMutex m_mutex;
    QThread m_thread;
    QMap<int, XmlQueryJob> m_jobs;
    QAtomicInt m_queryIds;
};

Q_GLOBAL_STATIC(QDeclarative1XmlQuery, globalXmlQuery)

void QDeclarative1XmlQuery::doQueryJob(XmlQueryJob *currentJob, QDeclarative1XmlQueryResult *currentResult)
{
    Q_ASSERT(currentJob->queryId != -1);

    QString r;
    QXmlQuery query;
    QBuffer buffer(&currentJob->data);
    buffer.open(QIODevice::ReadOnly);
    query.bindVariable(QLatin1String("src"), &buffer);
    query.setQuery(currentJob->namespaces + currentJob->query);
    query.evaluateTo(&r);

    //always need a single root element
    QByteArray xml = "<dummy:items xmlns:dummy=\"http://qtsotware.com/dummy\">\n" + r.toUtf8() + "</dummy:items>";
    QBuffer b(&xml);
    b.open(QIODevice::ReadOnly);

    QString namespaces = QLatin1String("declare namespace dummy=\"http://qtsotware.com/dummy\";\n") + currentJob->namespaces;
    QString prefix = QLatin1String("doc($inputDocument)/dummy:items") +
                     currentJob->query.mid(currentJob->query.lastIndexOf(QLatin1Char('/')));

    //figure out how many items we are dealing with
    int count = -1;
    {
        QXmlResultItems result;
        QXmlQuery countquery;
        countquery.bindVariable(QLatin1String("inputDocument"), &b);
        countquery.setQuery(namespaces + QLatin1String("count(") + prefix + QLatin1Char(')'));
        countquery.evaluateTo(&result);
        QXmlItem item(result.next());
        if (item.isAtomicValue())
            count = item.toAtomicValue().toInt();
    }

    currentJob->data = xml;
    currentJob->prefix = namespaces + prefix + QLatin1Char('/');
    currentResult->size = (count > 0 ? count : 0);
}

void QDeclarative1XmlQuery::getValuesOfKeyRoles(const XmlQueryJob& currentJob, QStringList *values, QXmlQuery *query) const
{
    const QStringList &keysQueries = currentJob.keyRoleQueries;
    QString keysQuery;
    if (keysQueries.count() == 1)
        keysQuery = currentJob.prefix + keysQueries[0];
    else if (keysQueries.count() > 1)
        keysQuery = currentJob.prefix + QLatin1String("concat(") + keysQueries.join(QLatin1String(",")) + QLatin1String(")");

    if (!keysQuery.isEmpty()) {
        query->setQuery(keysQuery);
        QXmlResultItems resultItems;
        query->evaluateTo(&resultItems);
        QXmlItem item(resultItems.next());
        while (!item.isNull()) {
            values->append(item.toAtomicValue().toString());
            item = resultItems.next();
        }
    }
}

void QDeclarative1XmlQuery::addIndexToRangeList(QList<QDeclarative1XmlListRange> *ranges, int index) const {
    if (ranges->isEmpty())
        ranges->append(qMakePair(index, 1));
    else if (ranges->last().first + ranges->last().second == index)
        ranges->last().second += 1;
    else
        ranges->append(qMakePair(index, 1));
}

void QDeclarative1XmlQuery::doSubQueryJob(XmlQueryJob *currentJob, QDeclarative1XmlQueryResult *currentResult)
{
    Q_ASSERT(currentJob->queryId != -1);

    QBuffer b(&currentJob->data);
    b.open(QIODevice::ReadOnly);

    QXmlQuery subquery;
    subquery.bindVariable(QLatin1String("inputDocument"), &b);

    QStringList keyRoleResults;
    getValuesOfKeyRoles(*currentJob, &keyRoleResults, &subquery);

    // See if any values of key roles have been inserted or removed.

    if (currentJob->keyRoleResultsCache.isEmpty()) {
        currentResult->inserted << qMakePair(0, currentResult->size);
    } else {
        if (keyRoleResults != currentJob->keyRoleResultsCache) {
            QStringList temp;
            for (int i=0; i<currentJob->keyRoleResultsCache.count(); i++) {
                if (!keyRoleResults.contains(currentJob->keyRoleResultsCache[i]))
                    addIndexToRangeList(&currentResult->removed, i);
                else 
                    temp << currentJob->keyRoleResultsCache[i];
            }

            for (int i=0; i<keyRoleResults.count(); i++) {
                if (temp.count() == i || keyRoleResults[i] != temp[i]) {
                    temp.insert(i, keyRoleResults[i]);
                    addIndexToRangeList(&currentResult->inserted, i);
                }
            }
        }
    }
    currentResult->keyRoleResultsCache = keyRoleResults;

    // Get the new values for each role.
    //### we might be able to condense even further (query for everything in one go)
    const QStringList &queries = currentJob->roleQueries;
    for (int i = 0; i < queries.size(); ++i) {
        QList<QVariant> resultList;
        if (!queries[i].isEmpty()) {
            subquery.setQuery(currentJob->prefix + QLatin1String("(let $v := string(") + queries[i] + QLatin1String(") return if ($v) then ") + queries[i] + QLatin1String(" else \"\")"));
            if (subquery.isValid()) {
                QXmlResultItems resultItems;
                subquery.evaluateTo(&resultItems);
                QXmlItem item(resultItems.next());
                while (!item.isNull()) {
                    resultList << item.toAtomicValue(); //### we used to trim strings
                    item = resultItems.next();
                }
            } else {
                emit error(currentJob->roleQueryErrorId.at(i), queries[i]);
            }
        }
        //### should warn here if things have gone wrong.
        while (resultList.count() < currentResult->size)
            resultList << QVariant();
        currentResult->data << resultList;
        b.seek(0);
    }

    //this method is much slower, but works better for incremental loading
    /*for (int j = 0; j < m_size; ++j) {
        QList<QVariant> resultList;
        for (int i = 0; i < m_roleObjects->size(); ++i) {
            QDeclarative1XmlListModelRole *role = m_roleObjects->at(i);
            subquery.setQuery(m_prefix.arg(j+1) + role->query());
            if (role->isStringList()) {
                QStringList data;
                subquery.evaluateTo(&data);
                resultList << QVariant(data);
                //qDebug() << data;
            } else {
                QString s;
                subquery.evaluateTo(&s);
                if (role->isCData()) {
                    //un-escape
                    s.replace(QLatin1String("&lt;"), QLatin1String("<"));
                    s.replace(QLatin1String("&gt;"), QLatin1String(">"));
                    s.replace(QLatin1String("&amp;"), QLatin1String("&"));
                }
                resultList << s.trimmed();
                //qDebug() << s;
            }
            b.seek(0);
        }
        m_modelData << resultList;
    }*/
}

class QDeclarative1XmlListModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1XmlListModel)
public:
    QDeclarative1XmlListModelPrivate()
        : isComponentComplete(true), size(-1), highestRole(Qt::UserRole)
        , reply(0), status(QDeclarative1XmlListModel::Null), progress(0.0)
        , queryId(-1), roleObjects(), redirectCount(0) {}


    void notifyQueryStarted(bool remoteSource) {
        Q_Q(QDeclarative1XmlListModel);
        progress = remoteSource ? 0.0 : 1.0;
        status = QDeclarative1XmlListModel::Loading;
        errorString.clear();
        emit q->progressChanged(progress);
        emit q->statusChanged(status);
    }

    bool isComponentComplete;
    QUrl src;
    QString xml;
    QString query;
    QString namespaces;
    int size;
    QList<int> roles;
    QStringList roleNames;
    int highestRole;
    QNetworkReply *reply;
    QDeclarative1XmlListModel::Status status;
    QString errorString;
    qreal progress;
    int queryId;
    QStringList keyRoleResultsCache;
    QList<QDeclarative1XmlListModelRole *> roleObjects;
    static void append_role(QDeclarativeListProperty<QDeclarative1XmlListModelRole> *list, QDeclarative1XmlListModelRole *role);
    static void clear_role(QDeclarativeListProperty<QDeclarative1XmlListModelRole> *list);
    QList<QList<QVariant> > data;
    int redirectCount;
};


void QDeclarative1XmlListModelPrivate::append_role(QDeclarativeListProperty<QDeclarative1XmlListModelRole> *list, QDeclarative1XmlListModelRole *role)
{
    QDeclarative1XmlListModel *_this = qobject_cast<QDeclarative1XmlListModel *>(list->object);
    if (_this && role) {
        int i = _this->d_func()->roleObjects.count();
        _this->d_func()->roleObjects.append(role);
        if (_this->d_func()->roleNames.contains(role->name())) {
            qmlInfo(role) << QObject::tr("\"%1\" duplicates a previous role name and will be disabled.").arg(role->name());
            return;
        }
        _this->d_func()->roles.insert(i, _this->d_func()->highestRole);
        _this->d_func()->roleNames.insert(i, role->name());
        ++_this->d_func()->highestRole;
    }
}

//### clear needs to invalidate any cached data (in data table) as well
//    (and the model should emit the appropriate signals)
void QDeclarative1XmlListModelPrivate::clear_role(QDeclarativeListProperty<QDeclarative1XmlListModelRole> *list)
{
    QDeclarative1XmlListModel *_this = static_cast<QDeclarative1XmlListModel *>(list->object);
    _this->d_func()->roles.clear();
    _this->d_func()->roleNames.clear();
    _this->d_func()->roleObjects.clear();
}

/*!
    \qmlclass XmlListModel QDeclarative1XmlListModel
    \inqmlmodule QtQuick 1
    \ingroup qml-working-with-data
  \since QtQuick 1.0
    \brief The XmlListModel element is used to specify a read-only model using XPath expressions.

    XmlListModel is used to create a read-only model from XML data. It can be used as a data source
    for view elements (such as ListView, PathView, GridView) and other elements that interact with model
    data (such as \l Repeater).

    For example, if there is a XML document at http://www.mysite.com/feed.xml like this:

    \code
    <?xml version="1.0" encoding="utf-8"?>
    <rss version="2.0">
        ...
        <channel>
            <item>
                <title>A blog post</title>
                <pubDate>Sat, 07 Sep 2010 10:00:01 GMT</pubDate>
            </item>
            <item>
                <title>Another blog post</title>
                <pubDate>Sat, 07 Sep 2010 15:35:01 GMT</pubDate>
            </item>
        </channel>
    </rss>
    \endcode

    A XmlListModel could create a model from this data, like this:

    \qml
    import QtQuick 1.0

    XmlListModel {
        id: xmlModel
        source: "http://www.mysite.com/feed.xml"
        query: "/rss/channel/item"

        XmlRole { name: "title"; query: "title/string()" }
        XmlRole { name: "pubDate"; query: "pubDate/string()" }
    }
    \endqml

    The \l {XmlListModel::query}{query} value of "/rss/channel/item" specifies that the XmlListModel should generate
    a model item for each \c <item> in the XML document. 
    
    The XmlRole objects define the
    model item attributes. Here, each model item will have \c title and \c pubDate 
    attributes that match the \c title and \c pubDate values of its corresponding \c <item>.
    (See \l XmlRole::query for more examples of valid XPath expressions for XmlRole.)

    The model could be used in a ListView, like this:

    \qml
    ListView {
        width: 180; height: 300
        model: xmlModel
        delegate: Text { text: title + ": " + pubDate }
    }
    \endqml

    \image qml-xmllistmodel-example.png

    The XmlListModel data is loaded asynchronously, and \l status
    is set to \c XmlListModel.Ready when loading is complete.
    Note this means when XmlListModel is used for a view, the view is not
    populated until the model is loaded.


    \section2 Using key XML roles

    You can define certain roles as "keys" so that when reload() is called,
    the model will only add and refresh data that contains new values for
    these keys.

    For example, if above role for "pubDate" was defined like this instead:

    \qml
        XmlRole { name: "pubDate"; query: "pubDate/string()"; isKey: true }
    \endqml

    Then when reload() is called, the model will only add and reload
    items with a "pubDate" value that is not already 
    present in the model. 

    This is useful when displaying the contents of XML documents that
    are incrementally updated (such as RSS feeds) to avoid repainting the
    entire contents of a model in a view.

    If multiple key roles are specified, the model only adds and reload items
    with a combined value of all key roles that is not already present in
    the model.

    \sa {RSS News}
*/

QDeclarative1XmlListModel::QDeclarative1XmlListModel(QObject *parent)
    : QListModelInterface(*(new QDeclarative1XmlListModelPrivate), parent)
{
    connect(globalXmlQuery(), SIGNAL(queryCompleted(QDeclarative1XmlQueryResult)),
            this, SLOT(queryCompleted(QDeclarative1XmlQueryResult)));
    connect(globalXmlQuery(), SIGNAL(error(void*,QString)),
            this, SLOT(queryError(void*,QString)));
}

QDeclarative1XmlListModel::~QDeclarative1XmlListModel()
{
}

/*!
    \qmlproperty list<XmlRole> QtQuick1::XmlListModel::roles

    The roles to make available for this model.
*/
QDeclarativeListProperty<QDeclarative1XmlListModelRole> QDeclarative1XmlListModel::roleObjects()
{
    Q_D(QDeclarative1XmlListModel);
    QDeclarativeListProperty<QDeclarative1XmlListModelRole> list(this, d->roleObjects);
    list.append = &QDeclarative1XmlListModelPrivate::append_role;
    list.clear = &QDeclarative1XmlListModelPrivate::clear_role;
    return list;
}

QHash<int,QVariant> QDeclarative1XmlListModel::data(int index, const QList<int> &roles) const
{
    Q_D(const QDeclarative1XmlListModel);
    QHash<int, QVariant> rv;
    for (int i = 0; i < roles.size(); ++i) {
        int role = roles.at(i);
        int roleIndex = d->roles.indexOf(role);
        rv.insert(role, roleIndex == -1 ? QVariant() : d->data.value(roleIndex).value(index));
    }
    return rv;
}

QVariant QDeclarative1XmlListModel::data(int index, int role) const
{
    Q_D(const QDeclarative1XmlListModel);
    int roleIndex = d->roles.indexOf(role);
    return (roleIndex == -1) ? QVariant() : d->data.value(roleIndex).value(index);
}

/*!
    \qmlproperty int QtQuick1::XmlListModel::count
    The number of data entries in the model.
*/
int QDeclarative1XmlListModel::count() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->size;
}

QList<int> QDeclarative1XmlListModel::roles() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->roles;
}

QString QDeclarative1XmlListModel::toString(int role) const
{
    Q_D(const QDeclarative1XmlListModel);
    int index = d->roles.indexOf(role);
    if (index == -1)
        return QString();
    return d->roleNames.at(index);
}

/*!
    \qmlproperty url QtQuick1::XmlListModel::source
    The location of the XML data source.

    If both \c source and \l xml are set, \l xml is used.
*/
QUrl QDeclarative1XmlListModel::source() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->src;
}

void QDeclarative1XmlListModel::setSource(const QUrl &src)
{
    Q_D(QDeclarative1XmlListModel);
    if (d->src != src) {
        d->src = src;
        if (d->xml.isEmpty())   // src is only used if d->xml is not set
            reload();
        emit sourceChanged();
   }
}

/*!
    \qmlproperty string QtQuick1::XmlListModel::xml
    This property holds the XML data for this model, if set.

    The text is assumed to be UTF-8 encoded.

    If both \l source and \c xml are set, \c xml is used.
*/
QString QDeclarative1XmlListModel::xml() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->xml;
}

void QDeclarative1XmlListModel::setXml(const QString &xml)
{
    Q_D(QDeclarative1XmlListModel);
    if (d->xml != xml) {
        d->xml = xml;
        reload();
        emit xmlChanged();
    }
}

/*!
    \qmlproperty string QtQuick1::XmlListModel::query
    An absolute XPath query representing the base query for creating model items
    from this model's XmlRole objects. The query should start with '/' or '//'.
*/
QString QDeclarative1XmlListModel::query() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->query;
}

void QDeclarative1XmlListModel::setQuery(const QString &query)
{
    Q_D(QDeclarative1XmlListModel);
    if (!query.startsWith(QLatin1Char('/'))) {
        qmlInfo(this) << QCoreApplication::translate("QDeclarative1XmlRoleList", "An XmlListModel query must start with '/' or \"//\"");
        return;
    }

    if (d->query != query) {
        d->query = query;
        reload();
        emit queryChanged();
    }
}

/*!
    \qmlproperty string QtQuick1::XmlListModel::namespaceDeclarations
    The namespace declarations to be used in the XPath queries.

    The namespaces should be declared as in XQuery. For example, if a requested document
    at http://mysite.com/feed.xml uses the namespace "http://www.w3.org/2005/Atom",
    this can be declared as the default namespace:

    \qml
    XmlListModel {
        source: "http://mysite.com/feed.xml"
        query: "/feed/entry"
        namespaceDeclarations: "declare default element namespace 'http://www.w3.org/2005/Atom';"

        XmlRole { name: "title"; query: "title/string()" }
    }
    \endqml
*/
QString QDeclarative1XmlListModel::namespaceDeclarations() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->namespaces;
}

void QDeclarative1XmlListModel::setNamespaceDeclarations(const QString &declarations)
{
    Q_D(QDeclarative1XmlListModel);
    if (d->namespaces != declarations) {
        d->namespaces = declarations;
        reload();
        emit namespaceDeclarationsChanged();
    }
}

/*!
    \qmlmethod object QtQuick1::XmlListModel::get(int index)

    Returns the item at \a index in the model.

    For example, for a model like this:

    \qml
    XmlListModel {
        id: model
        source: "http://mysite.com/feed.xml"
        query: "/feed/entry"
        XmlRole { name: "title"; query: "title/string()" }
    }
    \endqml

    This will access the \c title value for the first item in the model:

    \js
    var title = model.get(0).title;
    \endjs
*/
QDeclarativeV8Handle QDeclarative1XmlListModel::get(int index) const
{
    // Must be called with a context and handle scope
    Q_D(const QDeclarative1XmlListModel);

    if (index < 0 || index >= count())
        return QDeclarativeV8Handle::fromHandle(v8::Undefined());

    QDeclarativeEngine *engine = qmlContext(this)->engine();
    QV8Engine *v8engine = QDeclarativeEnginePrivate::getV8Engine(engine);
    v8::Local<v8::Object> rv = v8::Object::New();
    for (int ii = 0; ii < d->roleObjects.count(); ++ii) 
        rv->Set(v8engine->toString(d->roleObjects[ii]->name()), 
                v8engine->fromVariant(d->data.value(ii).value(index)));

    return QDeclarativeV8Handle::fromHandle(rv);
}

/*!
    \qmlproperty enumeration QtQuick1::XmlListModel::status
    Specifies the model loading status, which can be one of the following:

    \list
    \o XmlListModel.Null - No XML data has been set for this model.
    \o XmlListModel.Ready - The XML data has been loaded into the model.
    \o XmlListModel.Loading - The model is in the process of reading and loading XML data.
    \o XmlListModel.Error - An error occurred while the model was loading. See errorString() for details
       about the error.
    \endlist

    \sa progress

*/
QDeclarative1XmlListModel::Status QDeclarative1XmlListModel::status() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->status;
}

/*!
    \qmlproperty real QtQuick1::XmlListModel::progress

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
qreal QDeclarative1XmlListModel::progress() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->progress;
}

/*!
    \qmlmethod void QtQuick1::XmlListModel::errorString()

    Returns a string description of the last error that occurred
    if \l status is XmlListModel::Error.
*/
QString QDeclarative1XmlListModel::errorString() const
{
    Q_D(const QDeclarative1XmlListModel);
    return d->errorString;
}

void QDeclarative1XmlListModel::classBegin()
{
    Q_D(QDeclarative1XmlListModel);
    d->isComponentComplete = false;
}

void QDeclarative1XmlListModel::componentComplete()
{
    Q_D(QDeclarative1XmlListModel);
    d->isComponentComplete = true;
    reload();
}

/*!
    \qmlmethod QtQuick1::XmlListModel::reload()

    Reloads the model.
    
    If no key roles have been specified, all existing model
    data is removed, and the model is rebuilt from scratch.

    Otherwise, items are only added if the model does not already
    contain items with matching key role values.
    
    \sa {Using key XML roles}, XmlRole::isKey
*/
void QDeclarative1XmlListModel::reload()
{
    Q_D(QDeclarative1XmlListModel);

    if (!d->isComponentComplete)
        return;

    globalXmlQuery()->abort(d->queryId);
    d->queryId = -1;

    if (d->size < 0)
        d->size = 0;

    if (d->reply) {
        d->reply->abort();
        if (d->reply) {
            // abort will generally have already done this (and more)
            d->reply->deleteLater();
            d->reply = 0;
        }
    }

    if (!d->xml.isEmpty()) {
        d->queryId = globalXmlQuery()->doQuery(d->query, d->namespaces, d->xml.toUtf8(), &d->roleObjects, d->keyRoleResultsCache);
        d->notifyQueryStarted(false);

    } else if (d->src.isEmpty()) {
        d->queryId = XMLLISTMODEL_CLEAR_ID;
        d->notifyQueryStarted(false);
        QTimer::singleShot(0, this, SLOT(dataCleared()));

    } else {
        d->notifyQueryStarted(true);
        QNetworkRequest req(d->src);
        req.setRawHeader("Accept", "application/xml,*/*");
        d->reply = qmlContext(this)->engine()->networkAccessManager()->get(req);
        QObject::connect(d->reply, SIGNAL(finished()), this, SLOT(requestFinished()));
        QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)),
                         this, SLOT(requestProgress(qint64,qint64)));
    }
}

#define XMLLISTMODEL_MAX_REDIRECT 16

void QDeclarative1XmlListModel::requestFinished()
{
    Q_D(QDeclarative1XmlListModel);

    d->redirectCount++;
    if (d->redirectCount < XMLLISTMODEL_MAX_REDIRECT) {
        QVariant redirect = d->reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = d->reply->url().resolved(redirect.toUrl());
            d->reply->deleteLater();
            d->reply = 0;
            setSource(url);
            return;
        }
    }
    d->redirectCount = 0;

    if (d->reply->error() != QNetworkReply::NoError) {
        d->errorString = d->reply->errorString();
        disconnect(d->reply, 0, this, 0);
        d->reply->deleteLater();
        d->reply = 0;

        int count = this->count();
        d->data.clear();
        d->size = 0;
        if (count > 0) {
            emit itemsRemoved(0, count);
            emit countChanged();
        }

        d->status = Error;
        d->queryId = -1;
        emit statusChanged(d->status);
    } else {
        QByteArray data = d->reply->readAll();
        if (data.isEmpty()) {
            d->queryId = XMLLISTMODEL_CLEAR_ID;
            QTimer::singleShot(0, this, SLOT(dataCleared()));
        } else {
            d->queryId = globalXmlQuery()->doQuery(d->query, d->namespaces, data, &d->roleObjects, d->keyRoleResultsCache);
        }
        disconnect(d->reply, 0, this, 0);
        d->reply->deleteLater();
        d->reply = 0;

        d->progress = 1.0;
        emit progressChanged(d->progress);
    }
}

void QDeclarative1XmlListModel::requestProgress(qint64 received, qint64 total)
{
    Q_D(QDeclarative1XmlListModel);
    if (d->status == Loading && total > 0) {
        d->progress = qreal(received)/total;
        emit progressChanged(d->progress);
    }
}

void QDeclarative1XmlListModel::dataCleared()
{
    Q_D(QDeclarative1XmlListModel);
    QDeclarative1XmlQueryResult r;
    r.queryId = XMLLISTMODEL_CLEAR_ID;
    r.size = 0;
    r.removed << qMakePair(0, count());
    r.keyRoleResultsCache = d->keyRoleResultsCache;
    queryCompleted(r);
}

void QDeclarative1XmlListModel::queryError(void* object, const QString& error)
{
    // Be extra careful, object may no longer exist, it's just an ID.
    Q_D(QDeclarative1XmlListModel);
    for (int i=0; i<d->roleObjects.count(); i++) {
        if (d->roleObjects.at(i) == static_cast<QDeclarative1XmlListModelRole*>(object)) {
            qmlInfo(d->roleObjects.at(i)) << QObject::tr("invalid query: \"%1\"").arg(error);
            return;
        }
    }
    qmlInfo(this) << QObject::tr("invalid query: \"%1\"").arg(error);
}

void QDeclarative1XmlListModel::queryCompleted(const QDeclarative1XmlQueryResult &result)
{
    Q_D(QDeclarative1XmlListModel);
    if (result.queryId != d->queryId)
        return;

    int origCount = d->size;
    bool sizeChanged = result.size != d->size;

    d->size = result.size;
    d->data = result.data;
    d->keyRoleResultsCache = result.keyRoleResultsCache;
    d->status = Ready;
    d->errorString.clear();
    d->queryId = -1;

    bool hasKeys = false;
    for (int i=0; i<d->roleObjects.count(); i++) {
        if (d->roleObjects[i]->isKey()) {
            hasKeys = true;
            break;
        }
    }
    if (!hasKeys) {
        if (!(origCount == 0 && d->size == 0)) {
            emit itemsRemoved(0, origCount);
            emit itemsInserted(0, d->size);
            emit countChanged();
        }

    } else {
        for (int i=0; i<result.removed.count(); i++)
            emit itemsRemoved(result.removed[i].first, result.removed[i].second);
        for (int i=0; i<result.inserted.count(); i++)
            emit itemsInserted(result.inserted[i].first, result.inserted[i].second);

        if (sizeChanged)
            emit countChanged();
    }

    emit statusChanged(d->status);
}



QT_END_NAMESPACE

#include <qdeclarativexmllistmodel.moc>
