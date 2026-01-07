// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKELLIPSESHAPE_P_H
#define QQUICKELLIPSESHAPE_P_H

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

#include <QtQuickShapes/private/qquickshape_p.h>
#include <QtQuickShapesDesignHelpers/qtquickshapesdesignhelpersexports.h>

QT_BEGIN_NAMESPACE

class QQuickEllipseShapePrivate;

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickEllipseShape : public QQuickShape
{
public:
    Q_OBJECT
    Q_PROPERTY(qreal sweepAngle READ sweepAngle WRITE setSweepAngle NOTIFY sweepAngleChanged FINAL)
    Q_PROPERTY(qreal startAngle READ startAngle WRITE setStartAngle NOTIFY startAngleChanged FINAL)
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged FINAL)
    Q_PROPERTY(qreal innerArcRatio READ innerArcRatio WRITE setInnerArcRatio NOTIFY
                       innerArcRatioChanged FINAL)
    Q_PROPERTY(qreal cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY cornerRadiusChanged
                       FINAL)
    Q_PROPERTY(
            qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged FINAL)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged
                       FINAL)
    Q_PROPERTY(QQuickShapePath::CapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY
                       capStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::JoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY
                       joinStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::StrokeStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY
                       strokeStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::FillRule fillRule READ fillRule WRITE setFillRule NOTIFY
                       fillRuleChanged FINAL)
    Q_PROPERTY(QList<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY
                       dashPatternChanged FINAL)
    Q_PROPERTY(QQuickShapeGradient *fillGradient READ fillGradient WRITE setFillGradient NOTIFY
                       gradientChanged RESET resetFillGradient FINAL)
    Q_PROPERTY(BorderMode borderMode READ borderMode WRITE setBorderMode NOTIFY borderModeChanged
                       RESET resetBorderMode FINAL)

    QML_NAMED_ELEMENT(EllipseShape)
    QML_ADDED_IN_VERSION(6, 11)

public:
    QQuickEllipseShape(QQuickItem *parent = nullptr);
    ~QQuickEllipseShape() override;

    qreal sweepAngle() const;
    void setSweepAngle(qreal sweepAngle);

    qreal startAngle() const;
    void setStartAngle(qreal startAngle);

    qreal dashOffset() const;
    void setDashOffset(qreal offset);

    qreal innerArcRatio() const;
    void setInnerArcRatio(qreal innerArcRatio);

    qreal cornerRadius() const;
    void setCornerRadius(qreal cornerRadius);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal width);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    QQuickShapePath::CapStyle capStyle() const;
    void setCapStyle(QQuickShapePath::CapStyle style);

    QQuickShapePath::JoinStyle joinStyle() const;
    void setJoinStyle(QQuickShapePath::JoinStyle style);

    QQuickShapePath::StrokeStyle strokeStyle() const;
    void setStrokeStyle(QQuickShapePath::StrokeStyle style);

    QQuickShapePath::FillRule fillRule() const;
    void setFillRule(QQuickShapePath::FillRule fillRule);

    QList<qreal> dashPattern() const;
    void setDashPattern(const QList<qreal> &array);

    QQuickShapeGradient *fillGradient() const;
    void setFillGradient(QQuickShapeGradient *fillGradient);
    void resetFillGradient();

    enum class BorderMode { Inside, Middle, Outside };
    Q_ENUM(BorderMode)
    BorderMode borderMode() const;
    void setBorderMode(BorderMode borderMode);
    void resetBorderMode();

Q_SIGNALS:
    void innerArcRatioChanged();
    void cornerRadiusChanged();
    void startAngleChanged();
    void sweepAngleChanged();
    void strokeColorChanged();
    void strokeWidthChanged();
    void fillColorChanged();
    void joinStyleChanged();
    void capStyleChanged();
    void fillRuleChanged();
    void strokeStyleChanged();
    void dashOffsetChanged();
    void dashPatternChanged();
    void gradientChanged();
    void borderModeChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    Q_DISABLE_COPY(QQuickEllipseShape)
    Q_DECLARE_PRIVATE(QQuickEllipseShape)
};

QT_END_NAMESPACE

#endif // QQUICKELLIPSE1SHAPE_P_H
