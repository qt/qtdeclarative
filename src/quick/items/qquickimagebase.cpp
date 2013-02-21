/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickimagebase_p.h"
#include "qquickimagebase_p_p.h"

#include <QtQml/qqmlinfo.h>

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

    if (url == d->url)
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
    setSourceSize(QSize());
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
        if (d->progress != 0.0) {
            d->progress = 0.0;
            emit progressChanged(d->progress);
        }
        pixmapChange();
        d->status = Null;
        emit statusChanged(d->status);

        if (sourceSize() != d->oldSourceSize) {
            d->oldSourceSize = sourceSize();
            emit sourceSizeChanged();
        }
        update();

    } else {
        QQuickPixmap::Options options;
        if (d->async)
            options |= QQuickPixmap::Asynchronous;
        if (d->cache)
            options |= QQuickPixmap::Cache;
        d->pix.clear(this);
        d->pix.load(qmlEngine(this), d->url, d->sourcesize, options);

        if (d->pix.isLoading()) {
            if (d->progress != 0.0) {
                d->progress = 0.0;
                emit progressChanged(d->progress);
            }
            if (d->status != Loading) {
                d->status = Loading;
                emit statusChanged(d->status);
            }

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
            update(); //pixmap may have invalidated texture, updatePaintNode needs to be called before the next repaint
        } else {
            requestFinished();
        }
    }
}

void QQuickImageBase::requestFinished()
{
    Q_D(QQuickImageBase);

    if (d->pix.isError()) {
        qmlInfo(this) << d->pix.error();
        d->pix.clear(this);
        d->status = Error;
        if (d->progress != 0.0) {
            d->progress = 0.0;
            emit progressChanged(d->progress);
        }
    } else {
        d->status = Ready;
        if (d->progress != 1.0) {
            d->progress = 1.0;
            emit progressChanged(d->progress);
        }
    }
    pixmapChange();
    emit statusChanged(d->status);
    if (sourceSize() != d->oldSourceSize) {
        d->oldSourceSize = sourceSize();
        emit sourceSizeChanged();
    }
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
