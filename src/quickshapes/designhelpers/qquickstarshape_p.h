// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKSTARSHAPE_P_H
#define QQUICKSTARSHAPE_P_H

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

class QQuickStarShapePrivate;

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickStarShape : public QQuickShape
{
public:
    Q_OBJECT
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged FINAL)
    Q_PROPERTY(qreal cornerRadius READ cornerRadius WRITE setCornerRadius NOTIFY cornerRadiusChanged
                       FINAL)
    Q_PROPERTY(int pointCount READ pointCount WRITE setPointCount NOTIFY pointCountChanged FINAL)
    Q_PROPERTY(qreal ratio READ ratio WRITE setRatio NOTIFY ratioChanged FINAL)
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
    Q_PROPERTY(QList<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY
                       dashPatternChanged FINAL)
    Q_PROPERTY(QQuickShapeGradient *fillGradient READ fillGradient WRITE setFillGradient NOTIFY
                       gradientChanged RESET resetFillGradient FINAL)

    QML_NAMED_ELEMENT(StarShape)
    QML_ADDED_IN_VERSION(6, 11)

public:
    QQuickStarShape(QQuickItem *parent = nullptr);
    ~QQuickStarShape() override;

    qreal dashOffset() const;
    void setDashOffset(qreal);

    qreal cornerRadius() const;
    void setCornerRadius(qreal);

    qreal ratio() const;
    void setRatio(qreal);

    int pointCount() const;
    void setPointCount(int);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal);

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

    QList<qreal> dashPattern() const;
    void setDashPattern(const QList<qreal> &array);

    QQuickShapeGradient *fillGradient() const;
    void setFillGradient(QQuickShapeGradient *fillGradient);
    void resetFillGradient();

Q_SIGNALS:
    void cornerRadiusChanged();
    void ratioChanged();
    void pointCountChanged();
    void strokeColorChanged();
    void strokeWidthChanged();
    void fillColorChanged();
    void joinStyleChanged();
    void capStyleChanged();
    void strokeStyleChanged();
    void dashOffsetChanged();
    void dashPatternChanged();
    void gradientChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    Q_DISABLE_COPY(QQuickStarShape)
    Q_DECLARE_PRIVATE(QQuickStarShape)
};

QT_END_NAMESPACE

#endif // QQUICKSTARSHAPE_P_H
