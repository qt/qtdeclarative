/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKPATHITEM_P_H
#define QQUICKPATHITEM_P_H

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

#include "qquickitem.h"

#include <private/qtquickglobal_p.h>
#include <private/qquickpath_p.h>
#include <private/qv8engine_p.h>
#include <QGradientStops>

QT_REQUIRE_CONFIG(quick_path);

QT_BEGIN_NAMESPACE

class QQuickVisualPathPrivate;
class QQuickPathItemPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickPathGradientStop : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal position READ position WRITE setPosition)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    QQuickPathGradientStop(QObject *parent = nullptr);

    qreal position() const;
    void setPosition(qreal position);

    QColor color() const;
    void setColor(const QColor &color);

private:
    qreal m_position;
    QColor m_color;
};

class Q_QUICK_PRIVATE_EXPORT QQuickPathGradient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> stops READ stops)
    Q_PROPERTY(SpreadMode spread READ spread WRITE setSpread NOTIFY spreadChanged)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    enum SpreadMode {
        PadSpread,
        RepeatSpread,
        ReflectSpread
    };
    Q_ENUM(SpreadMode)

    QQuickPathGradient(QObject *parent = nullptr);

    QQmlListProperty<QObject> stops();

    QGradientStops sortedGradientStops() const;

    SpreadMode spread() const;
    void setSpread(SpreadMode mode);

signals:
    void updated();
    void spreadChanged();

private:
    static void appendStop(QQmlListProperty<QObject> *list, QObject *stop);

    QVector<QObject *> m_stops;
    SpreadMode m_spread;
};

class Q_QUICK_PRIVATE_EXPORT QQuickPathLinearGradient : public QQuickPathGradient
{
    Q_OBJECT
    Q_PROPERTY(qreal x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(qreal y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(qreal x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(qreal y2 READ y2 WRITE setY2 NOTIFY y2Changed)
    Q_CLASSINFO("DefaultProperty", "stops")

public:
    QQuickPathLinearGradient(QObject *parent = nullptr);

    qreal x1() const;
    void setX1(qreal v);
    qreal y1() const;
    void setY1(qreal v);
    qreal x2() const;
    void setX2(qreal v);
    qreal y2() const;
    void setY2(qreal v);

signals:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();

private:
    QPointF m_start;
    QPointF m_end;
};

class Q_QUICK_PRIVATE_EXPORT QQuickVisualPath : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickPath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_CLASSINFO("DefaultProperty", "path")

    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(FillRule fillRule READ fillRule WRITE setFillRule NOTIFY fillRuleChanged)
    Q_PROPERTY(JoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY joinStyleChanged)
    Q_PROPERTY(int miterLimit READ miterLimit WRITE setMiterLimit NOTIFY miterLimitChanged)
    Q_PROPERTY(CapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged)
    Q_PROPERTY(StrokeStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY strokeStyleChanged)
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged)
    Q_PROPERTY(QVector<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY dashPatternChanged)
    Q_PROPERTY(QQuickPathGradient *fillGradient READ fillGradient WRITE setFillGradient RESET resetFillGradient)

public:
    enum FillRule {
        OddEvenFill = Qt::OddEvenFill,
        WindingFill = Qt::WindingFill
    };
    Q_ENUM(FillRule)

    enum JoinStyle {
        MiterJoin = Qt::MiterJoin,
        BevelJoin = Qt::BevelJoin,
        RoundJoin = Qt::RoundJoin
    };
    Q_ENUM(JoinStyle)

    enum CapStyle {
        FlatCap = Qt::FlatCap,
        SquareCap = Qt::SquareCap,
        RoundCap = Qt::RoundCap
    };
    Q_ENUM(CapStyle)

    enum StrokeStyle {
        SolidLine = Qt::SolidLine,
        DashLine = Qt::DashLine
    };
    Q_ENUM(StrokeStyle)

    QQuickVisualPath(QObject *parent = nullptr);
    ~QQuickVisualPath();

    QQuickPath *path() const;
    void setPath(QQuickPath *path);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal w);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    FillRule fillRule() const;
    void setFillRule(FillRule fillRule);

    JoinStyle joinStyle() const;
    void setJoinStyle(JoinStyle style);

    int miterLimit() const;
    void setMiterLimit(int limit);

    CapStyle capStyle() const;
    void setCapStyle(CapStyle style);

    StrokeStyle strokeStyle() const;
    void setStrokeStyle(StrokeStyle style);

    qreal dashOffset() const;
    void setDashOffset(qreal offset);

    QVector<qreal> dashPattern() const;
    void setDashPattern(const QVector<qreal> &array);

    QQuickPathGradient *fillGradient() const;
    void setFillGradient(QQuickPathGradient *gradient);
    void resetFillGradient();

Q_SIGNALS:
    void changed();
    void pathChanged();
    void strokeColorChanged();
    void strokeWidthChanged();
    void fillColorChanged();
    void fillRuleChanged();
    void joinStyleChanged();
    void miterLimitChanged();
    void capStyleChanged();
    void strokeStyleChanged();
    void dashOffsetChanged();
    void dashPatternChanged();
    void fillGradientChanged();

private:
    Q_DISABLE_COPY(QQuickVisualPath)
    Q_DECLARE_PRIVATE(QQuickVisualPath)
    Q_PRIVATE_SLOT(d_func(), void _q_pathChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_fillGradientChanged())
};

class Q_QUICK_PRIVATE_EXPORT QQuickPathItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(RendererType renderer READ rendererType NOTIFY rendererChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(bool enableVendorExtensions READ enableVendorExtensions WRITE setEnableVendorExtensions NOTIFY enableVendorExtensionsChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QQmlListProperty<QQuickVisualPath> elements READ elements)
    Q_CLASSINFO("DefaultProperty", "elements")

public:
    enum RendererType {
        UnknownRenderer,
        GeometryRenderer,
        NvprRenderer,
        SoftwareRenderer
    };
    Q_ENUM(RendererType)

    enum Status {
        Null,
        Ready,
        Processing
    };
    Q_ENUM(Status)

    QQuickPathItem(QQuickItem *parent = nullptr);
    ~QQuickPathItem();

    RendererType rendererType() const;

    bool asynchronous() const;
    void setAsynchronous(bool async);

    bool enableVendorExtensions() const;
    void setEnableVendorExtensions(bool enable);

    Status status() const;

    QQmlListProperty<QQuickVisualPath> elements();

    Q_INVOKABLE void newPath(QQmlV4Function *args);
    Q_INVOKABLE void newStrokeFillParams(QQmlV4Function *args);
    Q_INVOKABLE void clearVisualPaths(QQmlV4Function *args);
    Q_INVOKABLE void commitVisualPaths(QQmlV4Function *args);
    Q_INVOKABLE void appendVisualPath(QQmlV4Function *args);

protected:
    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *) override;
    void updatePolish() override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void componentComplete() override;
    void classBegin() override;

Q_SIGNALS:
    void rendererChanged();
    void asynchronousChanged();
    void enableVendorExtensionsChanged();
    void statusChanged();

private:
    Q_DISABLE_COPY(QQuickPathItem)
    Q_DECLARE_PRIVATE(QQuickPathItem)
    Q_PRIVATE_SLOT(d_func(), void _q_visualPathChanged())
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPathItem)

#endif // QQUICKPATHITEM_P_H
