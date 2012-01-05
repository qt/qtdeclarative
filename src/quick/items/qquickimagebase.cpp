/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qquickimagebase_p.h"
#include "qquickimagebase_p_p.h"

#include <QtDeclarative/qdeclarativeinfo.h>

QT_BEGIN_NAMESPACE

QQuickImageBase::QQuickImageBase(QQuickItem *parent)
: QQuickImplicitSizeItem(*(new QQuickImageBasePrivate), parent)
{
    setFlag(ItemHasContents);
}

QQuickImageBase::QQuickImageBase(QQuickImageBasePrivate &dd, QQuickItem *parent)
: QQuickImplicitSizeItem(dd, parent)
{
    setFlag(ItemHasContents);
}

QQuickImageBase::~QQuickImageBase()
{
}

QQuickImageBase::Status QQuickImageBase::status() const
{
    Q_D(const QQuickImageBase);
    return d->status;
}


qreal QQuickImageBase::progress() const
{
    Q_D(const QQuickImageBase);
    return d->progress;
}


bool QQuickImageBase::asynchronous() const
{
    Q_D(const QQuickImageBase);
    return d->async;
}

void QQuickImageBase::setAsynchronous(bool async)
{
    Q_D(QQuickImageBase);
    if (d->async != async) {
        d->async = async;
        emit asynchronousChanged();
    }
}

QUrl QQuickImageBase::source() const
{
    Q_D(const QQuickImageBase);
    return d->url;
}

void QQuickImageBase::setSource(const QUrl &url)
{
    Q_D(QQuickImageBase);
    //equality is fairly expensive, so we bypass for simple, common case
    if ((d->url.isEmpty() == url.isEmpty()) && url == d->url)
        return;

    d->url = url;
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QQuickImageBase::setSourceSize(const QSize& size)
{
    Q_D(QQuickImageBase);
    if (d->sourcesize == size)
        return;

    d->sourcesize = size;
    d->explicitSourceSize = true;
    emit sourceSizeChanged();
    if (isComponentComplete())
        load();
}

QSize QQuickImageBase::sourceSize() const
{
    Q_D(const QQuickImageBase);

    int width = d->sourcesize.width();
    int height = d->sourcesize.height();
    return QSize(width != -1 ? width : d->pix.width(), height != -1 ? height : d->pix.height());
}

void QQuickImageBase::resetSourceSize()
{
    Q_D(QQuickImageBase);
    if (!d->explicitSourceSize)
        return;
    d->explicitSourceSize = false;
    d->sourcesize = QSize();
    emit sourceSizeChanged();
    if (isComponentComplete())
        load();
}

bool QQuickImageBase::cache() const
{
    Q_D(const QQuickImageBase);
    return d->cache;
}

void QQuickImageBase::setCache(bool cache)
{
    Q_D(QQuickImageBase);
    if (d->cache == cache)
        return;

    d->cache = cache;
    emit cacheChanged();
    if (isComponentComplete())
        load();
}

QImage QQuickImageBase::image() const
{
    Q_D(const QQuickImageBase);
    return d->pix.image();
}

void QQuickImageBase::setMirror(bool mirror)
{
    Q_D(QQuickImageBase);
    if (mirror == d->mirror)
        return;

    d->mirror = mirror;

    if (isComponentComplete())
        update();

    emit mirrorChanged();
}

bool QQuickImageBase::mirror() const
{
    Q_D(const QQuickImageBase);
    return d->mirror;
}

void QQuickImageBase::load()
{
    Q_D(QQuickImageBase);

    if (d->url.isEmpty()) {
        d->pix.clear(this);
        d->status = Null;
        d->progress = 0.0;
        pixmapChange();
        emit progressChanged(d->progress);
        emit statusChanged(d->status);
        update();
    } else {
        QDeclarativePixmap::Options options;
        if (d->async)
            options |= QDeclarativePixmap::Asynchronous;
        if (d->cache)
            options |= QDeclarativePixmap::Cache;
        d->pix.clear(this);
        pixmapChange();
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
                    QQuickImageBase::staticMetaObject.indexOfSlot("requestProgress(qint64,qint64)");
                thisRequestFinished =
                    QQuickImageBase::staticMetaObject.indexOfSlot("requestFinished()");
            }

            d->pix.connectFinished(this, thisRequestFinished);
            d->pix.connectDownloadProgress(this, thisRequestProgress);

        } else {
            requestFinished();
        }
    }
}

void QQuickImageBase::requestFinished()
{
    Q_D(QQuickImageBase);

    QQuickImageBase::Status oldStatus = d->status;
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

void QQuickImageBase::requestProgress(qint64 received, qint64 total)
{
    Q_D(QQuickImageBase);
    if (d->status == Loading && total > 0) {
        d->progress = qreal(received)/total;
        emit progressChanged(d->progress);
    }
}

void QQuickImageBase::componentComplete()
{
    Q_D(QQuickImageBase);
    QQuickItem::componentComplete();
    if (d->url.isValid())
        load();
}

void QQuickImageBase::pixmapChange()
{
    Q_D(QQuickImageBase);
    setImplicitSize(d->pix.width(), d->pix.height());
}

QT_END_NAMESPACE
