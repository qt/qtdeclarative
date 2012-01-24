/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativeimagebase_p.h"
#include "QtQuick1/private/qdeclarativeimagebase_p_p.h"

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtQuick1/private/qdeclarativepixmapcache_p.h>

QT_BEGIN_NAMESPACE



QDeclarative1ImageBase::QDeclarative1ImageBase(QDeclarativeItem *parent)
  : QDeclarative1ImplicitSizeItem(*(new QDeclarative1ImageBasePrivate), parent)
{
}

QDeclarative1ImageBase::QDeclarative1ImageBase(QDeclarative1ImageBasePrivate &dd, QDeclarativeItem *parent)
  : QDeclarative1ImplicitSizeItem(dd, parent)
{
}

QDeclarative1ImageBase::~QDeclarative1ImageBase()
{
}

QDeclarative1ImageBase::Status QDeclarative1ImageBase::status() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->status;
}


qreal QDeclarative1ImageBase::progress() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->progress;
}


bool QDeclarative1ImageBase::asynchronous() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->async;
}

void QDeclarative1ImageBase::setAsynchronous(bool async)
{
    Q_D(QDeclarative1ImageBase);
    if (d->async != async) {
        d->async = async;
        emit asynchronousChanged();
    }
}

QUrl QDeclarative1ImageBase::source() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->url;
}

void QDeclarative1ImageBase::setSource(const QUrl &url)
{
    Q_D(QDeclarative1ImageBase);
    //equality is fairly expensive, so we bypass for simple, common case
    if ((d->url.isEmpty() == url.isEmpty()) && url == d->url)
        return;

    d->url = url;
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QDeclarative1ImageBase::setSourceSize(const QSize& size)
{
    Q_D(QDeclarative1ImageBase);
    if (d->sourcesize == size)
        return;

    d->sourcesize = size;
    d->explicitSourceSize = true;
    emit sourceSizeChanged();
    if (isComponentComplete())
        load();
}

QSize QDeclarative1ImageBase::sourceSize() const
{
    Q_D(const QDeclarative1ImageBase);

    int width = d->sourcesize.width();
    int height = d->sourcesize.height();
    return QSize(width != -1 ? width : d->pix.width(), height != -1 ? height : d->pix.height());
}

void QDeclarative1ImageBase::resetSourceSize()
{
    Q_D(QDeclarative1ImageBase);
    if (!d->explicitSourceSize)
        return;
    d->explicitSourceSize = false;
    d->sourcesize = QSize();
    emit sourceSizeChanged();
    if (isComponentComplete())
        load();
}

bool QDeclarative1ImageBase::cache() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->cache;
}

void QDeclarative1ImageBase::setCache(bool cache)
{
    Q_D(QDeclarative1ImageBase);
    if (d->cache == cache)
        return;

    d->cache = cache;
    emit cacheChanged();
    if (isComponentComplete())
        load();
}

void QDeclarative1ImageBase::setMirror(bool mirror)
{
    Q_D(QDeclarative1ImageBase);
    if (mirror == d->mirror)
        return;

    d->mirror = mirror;

    if (isComponentComplete())
        update();

    emit mirrorChanged();
}

bool QDeclarative1ImageBase::mirror() const
{
    Q_D(const QDeclarative1ImageBase);
    return d->mirror;
}

void QDeclarative1ImageBase::load()
{
    Q_D(QDeclarative1ImageBase);

    if (d->url.isEmpty()) {
        d->pix.clear(this);
        d->status = Null;
        d->progress = 0.0;
        pixmapChange();
        emit progressChanged(d->progress);
        emit statusChanged(d->status);
        update();
    } else {
        QDeclarative1Pixmap::Options options;
        if (d->async)
            options |= QDeclarative1Pixmap::Asynchronous;
        if (d->cache)
            options |= QDeclarative1Pixmap::Cache;
        d->pix.clear(this);
        d->pix.load(qmlEngine(this), d->url, d->explicitSourceSize ? sourceSize() : QSize(), options);

        if (d->pix.isLoading()) {
            d->progress = 0.0;
            d->status = Loading;
            emit progressChanged(d->progress);
            emit statusChanged(d->status);

            static int thisRequestProgress = -1;
            static int thisRequestFinished = -1;
            if (thisRequestProgress == -1) {
                thisRequestProgress =
                    QDeclarative1ImageBase::staticMetaObject.indexOfSlot("requestProgress(qint64,qint64)");
                thisRequestFinished =
                    QDeclarative1ImageBase::staticMetaObject.indexOfSlot("requestFinished()");
            }

            d->pix.connectFinished(this, thisRequestFinished);
            d->pix.connectDownloadProgress(this, thisRequestProgress);

        } else {
            requestFinished();
        }
    }
}

void QDeclarative1ImageBase::requestFinished()
{
    Q_D(QDeclarative1ImageBase);

    QDeclarative1ImageBase::Status oldStatus = d->status;
    qreal oldProgress = d->progress;

    if (d->pix.isError()) {
        d->status = Error;
        qmlInfo(this) << d->pix.error();
    } else {
        d->status = Ready;
    }

    d->progress = 1.0;

    pixmapChange();

    if (d->sourcesize.width() != d->pix.width() || d->sourcesize.height() != d->pix.height())
        emit sourceSizeChanged();

    if (d->status != oldStatus)
        emit statusChanged(d->status);
    if (d->progress != oldProgress)
        emit progressChanged(d->progress);

    update();
}

void QDeclarative1ImageBase::requestProgress(qint64 received, qint64 total)
{
    Q_D(QDeclarative1ImageBase);
    if (d->status == Loading && total > 0) {
        d->progress = qreal(received)/total;
        emit progressChanged(d->progress);
    }
}

void QDeclarative1ImageBase::componentComplete()
{
    Q_D(QDeclarative1ImageBase);
    QDeclarativeItem::componentComplete();
    if (d->url.isValid())
        load();
}

void QDeclarative1ImageBase::pixmapChange()
{
    Q_D(QDeclarative1ImageBase);
    setImplicitWidth(d->pix.width());
    setImplicitHeight(d->pix.height());
}



QT_END_NAMESPACE
