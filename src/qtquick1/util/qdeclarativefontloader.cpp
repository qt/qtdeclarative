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

#include "QtQuick1/private/qdeclarativefontloader_p.h"

#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeengine.h>

#include <QStringList>
#include <QUrl>
#include <QDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFontDatabase>

#include <private/qobject_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>
#include <QtDeclarative/qdeclarativeinfo.h>

QT_BEGIN_NAMESPACE



#define FONTLOADER_MAXIMUM_REDIRECT_RECURSION 16

class QDeclarative1FontObject : public QObject
{
Q_OBJECT

public:
    QDeclarative1FontObject(int _id);

    void download(const QUrl &url, QNetworkAccessManager *manager);

Q_SIGNALS:
    void fontDownloaded(const QString&, QDeclarative1FontLoader::Status);

private Q_SLOTS:
    void replyFinished();

public:
    int id;

private:
    QNetworkReply *reply;
    int redirectCount;

    Q_DISABLE_COPY(QDeclarative1FontObject)
};

QDeclarative1FontObject::QDeclarative1FontObject(int _id = -1)
    : QObject(0), id(_id), reply(0), redirectCount(0) {}


void QDeclarative1FontObject::download(const QUrl &url, QNetworkAccessManager *manager)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
    reply = manager->get(req);
    QObject::connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
}

void QDeclarative1FontObject::replyFinished()
{
    if (reply) {
        redirectCount++;
        if (redirectCount < FONTLOADER_MAXIMUM_REDIRECT_RECURSION) {
            QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            if (redirect.isValid()) {
                QUrl url = reply->url().resolved(redirect.toUrl());
                QNetworkAccessManager *manager = reply->manager();
                reply->deleteLater();
                reply = 0;
                download(url, manager);
                return;
            }
        }
        redirectCount = 0;

        if (!reply->error()) {
            id = QFontDatabase::addApplicationFontFromData(reply->readAll());
            if (id != -1)
                emit fontDownloaded(QFontDatabase::applicationFontFamilies(id).at(0), QDeclarative1FontLoader::Ready);
            else
                emit fontDownloaded(QString(), QDeclarative1FontLoader::Error);
        } else {
            emit fontDownloaded(QString(), QDeclarative1FontLoader::Error);
        }
        reply->deleteLater();
        reply = 0;
    }
}


class QDeclarative1FontLoaderPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarative1FontLoader)

public:
    QDeclarative1FontLoaderPrivate() : status(QDeclarative1FontLoader::Null) {}

    QUrl url;
    QString name;
    QDeclarative1FontLoader::Status status;
    static QHash<QUrl, QDeclarative1FontObject*> fonts;
};

QHash<QUrl, QDeclarative1FontObject*> QDeclarative1FontLoaderPrivate::fonts;

/*!
    \qmlclass FontLoader QDeclarative1FontLoader
  \ingroup qml-utility-elements
    \since 4.7
    \brief The FontLoader element allows fonts to be loaded by name or URL.

    The FontLoader element is used to load fonts by name or URL. 
    
    The \l status indicates when the font has been loaded, which is useful 
    for fonts loaded from remote sources.

    For example:
    \qml
    import QtQuick 1.0

    Column { 
        FontLoader { id: fixedFont; name: "Courier" }
        FontLoader { id: webFont; source: "http://www.mysite.com/myfont.ttf" }

        Text { text: "Fixed-size font"; font.family: fixedFont.name }
        Text { text: "Fancy font"; font.family: webFont.name }
    }
    \endqml

    \sa {declarative/text/fonts}{Fonts example}
*/
QDeclarative1FontLoader::QDeclarative1FontLoader(QObject *parent)
    : QObject(*(new QDeclarative1FontLoaderPrivate), parent)
{
}

QDeclarative1FontLoader::~QDeclarative1FontLoader()
{
}

/*!
    \qmlproperty url FontLoader::source
    The url of the font to load.
*/
QUrl QDeclarative1FontLoader::source() const
{
    Q_D(const QDeclarative1FontLoader);
    return d->url;
}

void QDeclarative1FontLoader::setSource(const QUrl &url)
{
    Q_D(QDeclarative1FontLoader);
    if (url == d->url)
        return;
    d->url = qmlContext(this)->resolvedUrl(url);
    emit sourceChanged();

#ifndef QT_NO_LOCALFILE_OPTIMIZED_QML
    QString localFile = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(d->url);
    if (!localFile.isEmpty()) {
        if (!d->fonts.contains(d->url)) {
            int id = QFontDatabase::addApplicationFont(localFile);
            if (id != -1) {
                updateFontInfo(QFontDatabase::applicationFontFamilies(id).at(0), Ready);
                QDeclarative1FontObject *fo = new QDeclarative1FontObject(id);
                d->fonts[d->url] = fo;
            } else {
                updateFontInfo(QString(), Error);
            }
        } else {
            updateFontInfo(QFontDatabase::applicationFontFamilies(d->fonts[d->url]->id).at(0), Ready);
        }
    } else
#endif
    {
        if (!d->fonts.contains(d->url)) {
            QDeclarative1FontObject *fo = new QDeclarative1FontObject;
            d->fonts[d->url] = fo;
            fo->download(d->url, qmlEngine(this)->networkAccessManager());
            d->status = Loading;
            emit statusChanged();
            QObject::connect(fo, SIGNAL(fontDownloaded(QString,QDeclarative1FontLoader::Status)),
                this, SLOT(updateFontInfo(QString,QDeclarative1FontLoader::Status)));
        } else {
            QDeclarative1FontObject *fo = d->fonts[d->url];
            if (fo->id == -1) {
                d->status = Loading;
                emit statusChanged();
                QObject::connect(fo, SIGNAL(fontDownloaded(QString,QDeclarative1FontLoader::Status)),
                    this, SLOT(updateFontInfo(QString,QDeclarative1FontLoader::Status)));
            }
            else
                updateFontInfo(QFontDatabase::applicationFontFamilies(fo->id).at(0), Ready);
        }
    }
}

void QDeclarative1FontLoader::updateFontInfo(const QString& name, QDeclarative1FontLoader::Status status)
{
    Q_D(QDeclarative1FontLoader);

    if (name != d->name) {
        d->name = name;
        emit nameChanged();
    }
    if (status != d->status) {
        if (status == Error)
            qmlInfo(this) << "Cannot load font: \"" << d->url.toString() << "\"";
        d->status = status;
        emit statusChanged();
    }
}

/*!
    \qmlproperty string FontLoader::name

    This property holds the name of the font family.
    It is set automatically when a font is loaded using the \c url property.

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
QString QDeclarative1FontLoader::name() const
{
    Q_D(const QDeclarative1FontLoader);
    return d->name;
}

void QDeclarative1FontLoader::setName(const QString &name)
{
    Q_D(QDeclarative1FontLoader);
    if (d->name == name)
        return;
    d->name = name;
    emit nameChanged();
    d->status = Ready;
    emit statusChanged();
}

/*!
    \qmlproperty enumeration FontLoader::status

    This property holds the status of font loading.  It can be one of:
    \list
    \o FontLoader.Null - no font has been set
    \o FontLoader.Ready - the font has been loaded
    \o FontLoader.Loading - the font is currently being loaded
    \o FontLoader.Error - an error occurred while loading the font
    \endlist

    Use this status to provide an update or respond to the status change in some way.
    For example, you could:

    \list
    \o Trigger a state change:
    \qml
        State { name: 'loaded'; when: loader.status == FontLoader.Ready }
    \endqml

    \o Implement an \c onStatusChanged signal handler:
    \qml
        FontLoader {
            id: loader
            onStatusChanged: if (loader.status == FontLoader.Ready) console.log('Loaded')
        }
    \endqml

    \o Bind to the status value:
    \qml
        Text { text: loader.status == FontLoader.Ready ? 'Loaded' : 'Not loaded' }
    \endqml
    \endlist
*/
QDeclarative1FontLoader::Status QDeclarative1FontLoader::status() const
{
    Q_D(const QDeclarative1FontLoader);
    return d->status;
}



QT_END_NAMESPACE

#include <qdeclarativefontloader.moc>
