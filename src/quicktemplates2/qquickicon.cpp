/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickicon_p.h"
#include "qtaggedpointer.h"

#include <private/qqmlcontextdata_p.h>
#include <private/qqmldata_p.h>

QT_BEGIN_NAMESPACE

class QQuickIconPrivate : public QSharedData
{
public:
    // This is based on QFont's resolve_mask.
    enum ResolveProperties {
        NameResolved = 0x0001,
        SourceResolved = 0x0002,
        WidthResolved = 0x0004,
        HeightResolved = 0x0008,
        ColorResolved = 0x0010,
        CacheResolved = 0x0020,
        AllPropertiesResolved = 0x1ffff
    };
    int resolveMask = 0;

    QString name;
    QUrl source;
    int width = 0;
    int height = 0;
    QColor color = Qt::transparent;

    // we want DoCache as the default, and thus as the zero value
    // so that the tagged pointer can be zero initialized
    enum CacheStatus : bool { DoCache, SkipCaching };
    static_assert (DoCache == 0);
    /* We use a QTaggedPointer here to save space:
       - Without it, we would need an additional boolean, which due to
         alignment would increase the class size by sizeof(void *)
       - The pointer part stores the "owner" of the QQuickIcon, i.e.
         an object which has an icon property. We need the owner to
         access its context to resolve relative url's in the way users
         expect.
       - The tag bits are used to track whether caching is enabled.
     */
    QTaggedPointer<QObject, CacheStatus> ownerAndCache = nullptr;

};

QQuickIcon::QQuickIcon()
    : d(new QQuickIconPrivate)
{
}

QQuickIcon::QQuickIcon(const QQuickIcon &other)
    : d(other.d)
{
}

QQuickIcon::~QQuickIcon()
{
}

QQuickIcon &QQuickIcon::operator=(const QQuickIcon &other)
{
    d = other.d;
    return *this;
}

bool QQuickIcon::operator==(const QQuickIcon &other) const
{
    return d == other.d || (d->name == other.d->name
                            && d->source == other.d->source
                            && d->width == other.d->width
                            && d->height == other.d->height
                            && d->color == other.d->color
                            && d->ownerAndCache == other.d->ownerAndCache);
}

bool QQuickIcon::operator!=(const QQuickIcon &other) const
{
    return !(*this == other);
}

bool QQuickIcon::isEmpty() const
{
    return d->name.isEmpty() && d->source.isEmpty();
}

QString QQuickIcon::name() const
{
    return d->name;
}

void QQuickIcon::setName(const QString &name)
{
    if ((d->resolveMask & QQuickIconPrivate::NameResolved) && d->name == name)
        return;

    d.detach();
    d->name = name;
    d->resolveMask |= QQuickIconPrivate::NameResolved;
}

void QQuickIcon::resetName()
{
    d.detach();
    d->name = QString();
    d->resolveMask &= ~QQuickIconPrivate::NameResolved;
}

QUrl QQuickIcon::source() const
{
    return d->source;
}

void QQuickIcon::setSource(const QUrl &source)
{
    if ((d->resolveMask & QQuickIconPrivate::SourceResolved) && d->source == source)
        return;

    d.detach();
    d->source = source;
    d->resolveMask |= QQuickIconPrivate::SourceResolved;
}

void QQuickIcon::resetSource()
{
    d.detach();
    d->source = QString();
    d->resolveMask &= ~QQuickIconPrivate::SourceResolved;
}

QUrl QQuickIcon::resolvedSource() const
{
    if (QObject *owner = d->ownerAndCache.data()) {
        QQmlData *data = QQmlData::get(owner);
        if (data && data->outerContext)
            return data->outerContext->resolvedUrl(d->source);
    }

    return d->source;
}

int QQuickIcon::width() const
{
    return d->width;
}

void QQuickIcon::setWidth(int width)
{
    if ((d->resolveMask & QQuickIconPrivate::WidthResolved) && d->width == width)
        return;

    d.detach();
    d->width = width;
    d->resolveMask |= QQuickIconPrivate::WidthResolved;
}

void QQuickIcon::resetWidth()
{
    d.detach();
    d->width = 0;
    d->resolveMask &= ~QQuickIconPrivate::WidthResolved;
}

int QQuickIcon::height() const
{
    return d->height;
}

void QQuickIcon::setHeight(int height)
{
    if ((d->resolveMask & QQuickIconPrivate::HeightResolved) && d->height == height)
        return;

    d.detach();
    d->height = height;
    d->resolveMask |= QQuickIconPrivate::HeightResolved;
}

void QQuickIcon::resetHeight()
{
    d.detach();
    d->height = 0;
    d->resolveMask &= ~QQuickIconPrivate::HeightResolved;
}

QColor QQuickIcon::color() const
{
    return d->color;
}

void QQuickIcon::setColor(const QColor &color)
{
    if ((d->resolveMask & QQuickIconPrivate::ColorResolved) && d->color == color)
        return;

    d.detach();
    d->color = color;
    d->resolveMask |= QQuickIconPrivate::ColorResolved;
}

void QQuickIcon::resetColor()
{
    d.detach();
    d->color = Qt::transparent;
    d->resolveMask &= ~QQuickIconPrivate::ColorResolved;
}

bool QQuickIcon::cache() const
{
    return d->ownerAndCache.tag() == QQuickIconPrivate::DoCache;
}

void QQuickIcon::setCache(bool cache)
{
    const auto cacheState = cache ? QQuickIconPrivate::DoCache : QQuickIconPrivate::SkipCaching;
    if ((d->resolveMask & QQuickIconPrivate::CacheResolved) && d->ownerAndCache.tag() == cacheState)
        return;

    d.detach();
    d->ownerAndCache.setTag(cacheState);
    d->resolveMask |= QQuickIconPrivate::CacheResolved;
}

void QQuickIcon::resetCache()
{
    d.detach();
    d->ownerAndCache.setTag(QQuickIconPrivate::DoCache);
    d->resolveMask &= ~QQuickIconPrivate::CacheResolved;
}

void QQuickIcon::setOwner(QObject *owner)
{
    if (d->ownerAndCache.data() == owner)
        return;
    d.detach();
    d->ownerAndCache = owner;
}

QQuickIcon QQuickIcon::resolve(const QQuickIcon &other) const
{
    QQuickIcon resolved = *this;
    resolved.d.detach();

    if (!(d->resolveMask & QQuickIconPrivate::NameResolved))
        resolved.d->name = other.d->name;

    if (!(d->resolveMask & QQuickIconPrivate::SourceResolved))
        resolved.d->source = other.d->source;

    if (!(d->resolveMask & QQuickIconPrivate::WidthResolved))
        resolved.d->width = other.d->width;

    if (!(d->resolveMask & QQuickIconPrivate::HeightResolved))
        resolved.d->height = other.d->height;

    if (!(d->resolveMask & QQuickIconPrivate::ColorResolved))
        resolved.d->color = other.d->color;

    if (!(d->resolveMask & QQuickIconPrivate::CacheResolved))
        resolved.d->ownerAndCache.setTag(other.d->ownerAndCache.tag());

    // owner does not change when resolving an icon

    return resolved;
}

QT_END_NAMESPACE
