/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKSCALEGRID_P_P_H
#define QQUICKSCALEGRID_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickborderimage_p.h"

#include <QtQml/qqml.h>
#include <QtCore/qobject.h>

#include <QtQuick/private/qquickpixmapcache_p.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQuickScaleGrid : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int left READ left WRITE setLeft NOTIFY leftBorderChanged)
    Q_PROPERTY(int top READ top WRITE setTop NOTIFY topBorderChanged)
    Q_PROPERTY(int right READ right WRITE setRight NOTIFY rightBorderChanged)
    Q_PROPERTY(int bottom READ bottom WRITE setBottom NOTIFY bottomBorderChanged)
    QML_ANONYMOUS

public:
    QQuickScaleGrid(QObject *parent=nullptr);
    ~QQuickScaleGrid();

    bool isNull() const;

    int left() const { return _left; }
    void setLeft(int);

    int top() const { return _top; }
    void setTop(int);

    int right() const { return _right; }
    void setRight(int);

    int  bottom() const { return _bottom; }
    void setBottom(int);

Q_SIGNALS:
    void borderChanged();
    void leftBorderChanged();
    void topBorderChanged();
    void rightBorderChanged();
    void bottomBorderChanged();

private:
    int _left;
    int _top;
    int _right;
    int _bottom;
};

class Q_AUTOTEST_EXPORT QQuickGridScaledImage
{
public:
    QQuickGridScaledImage();
    QQuickGridScaledImage(const QQuickGridScaledImage &);
    QQuickGridScaledImage(QIODevice*);
    QQuickGridScaledImage &operator=(const QQuickGridScaledImage &);
    bool isValid() const;
    int gridLeft() const;
    int gridRight() const;
    int gridTop() const;
    int gridBottom() const;
    QQuickBorderImage::TileMode horizontalTileRule() const { return _h; }
    QQuickBorderImage::TileMode verticalTileRule() const { return _v; }

    QString pixmapUrl() const;

private:
    static QQuickBorderImage::TileMode stringToRule(const QStringRef &);

private:
    int _l;
    int _r;
    int _t;
    int _b;
    QQuickBorderImage::TileMode _h;
    QQuickBorderImage::TileMode _v;
    QString _pix;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickScaleGrid)

#endif // QQUICKSCALEGRID_P_P_H
