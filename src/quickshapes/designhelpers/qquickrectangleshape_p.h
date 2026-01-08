// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKRECTANGLESHAPE_P_H
#define QQUICKRECTANGLESHAPE_P_H

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

class QQuickRectangleShapePrivate;

class Q_QUICKSHAPESDESIGNHELPERS_EXPORT QQuickRectangleShape : public QQuickShape
{
    Q_OBJECT

    Q_PROPERTY(bool drawTop READ drawTop WRITE setDrawTop NOTIFY drawTopChanged RESET resetDrawTop FINAL REVISION(6, 11))
    Q_PROPERTY(bool drawRight READ drawRight WRITE setDrawRight NOTIFY drawRightChanged RESET resetDrawRight FINAL REVISION(6, 11))
    Q_PROPERTY(bool drawBottom READ drawBottom WRITE setDrawBottom NOTIFY drawBottomChanged RESET resetDrawBottom FINAL REVISION(6, 11))
    Q_PROPERTY(bool drawLeft READ drawLeft WRITE setDrawLeft NOTIFY drawLeftChanged RESET resetDrawLeft FINAL REVISION(6, 11))
    Q_PROPERTY(int radius READ radius WRITE setRadius NOTIFY radiusChanged FINAL)
    Q_PROPERTY(int topLeftRadius READ topLeftRadius WRITE setTopLeftRadius NOTIFY topLeftRadiusChanged RESET resetTopLeftRadius FINAL)
    Q_PROPERTY(int topRightRadius READ topRightRadius WRITE setTopRightRadius NOTIFY topRightRadiusChanged RESET resetTopRightRadius FINAL)
    Q_PROPERTY(int bottomLeftRadius READ bottomLeftRadius WRITE setBottomLeftRadius NOTIFY bottomLeftRadiusChanged RESET resetBottomLeftRadius FINAL)
    Q_PROPERTY(int bottomRightRadius READ bottomRightRadius WRITE setBottomRightRadius NOTIFY bottomRightRadiusChanged RESET resetBottomRightRadius FINAL)
    Q_PROPERTY(bool bevel READ hasBevel WRITE setBevel NOTIFY bevelChanged RESET resetBevel FINAL)
    Q_PROPERTY(bool topLeftBevel READ hasTopLeftBevel WRITE setTopLeftBevel NOTIFY topLeftBevelChanged RESET resetTopLeftBevel FINAL)
    Q_PROPERTY(bool topRightBevel READ hasTopRightBevel WRITE setTopRightBevel NOTIFY topRightBevelChanged RESET resetTopRightBevel FINAL)
    Q_PROPERTY(bool bottomLeftBevel READ hasBottomLeftBevel WRITE setBottomLeftBevel NOTIFY bottomLeftBevelChanged RESET resetBottomLeftBevel FINAL)
    Q_PROPERTY(bool bottomRightBevel READ hasBottomRightBevel WRITE setBottomRightBevel NOTIFY bottomRightBevelChanged RESET resetBottomRightBevel FINAL)
    Q_PROPERTY(QColor strokeColor READ strokeColor WRITE setStrokeColor NOTIFY strokeColorChanged FINAL)
    Q_PROPERTY(qreal strokeWidth READ strokeWidth WRITE setStrokeWidth NOTIFY strokeWidthChanged FINAL)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged FINAL)
    Q_PROPERTY(QQuickShapePath::JoinStyle joinStyle READ joinStyle WRITE setJoinStyle NOTIFY joinStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::CapStyle capStyle READ capStyle WRITE setCapStyle NOTIFY capStyleChanged FINAL)
    Q_PROPERTY(QQuickShapePath::StrokeStyle strokeStyle READ strokeStyle WRITE setStrokeStyle NOTIFY strokeStyleChanged FINAL)
    Q_PROPERTY(qreal dashOffset READ dashOffset WRITE setDashOffset NOTIFY dashOffsetChanged FINAL)
    Q_PROPERTY(QList<qreal> dashPattern READ dashPattern WRITE setDashPattern NOTIFY dashPatternChanged FINAL)
    Q_PROPERTY(QQuickShapeGradient *fillGradient READ fillGradient WRITE setFillGradient RESET resetFillGradient FINAL)
    Q_PROPERTY(BorderMode borderMode READ borderMode WRITE setBorderMode RESET resetBorderMode FINAL)

    QML_NAMED_ELEMENT(RectangleShape)
    QML_ADDED_IN_VERSION(6, 10)

public:
    QQuickRectangleShape(QQuickItem *parent = nullptr);
    ~QQuickRectangleShape();

    bool drawTop() const;
    void setDrawTop(bool drawTop);
    void resetDrawTop();

    bool drawRight() const;
    void setDrawRight(bool drawRight);
    void resetDrawRight();

    bool drawBottom() const;
    void setDrawBottom(bool drawBottom);
    void resetDrawBottom();

    bool drawLeft() const;
    void setDrawLeft(bool drawLeft);
    void resetDrawLeft();

    int radius() const;
    void setRadius(int radius);
    void resetRadius();

    int topLeftRadius() const;
    void setTopLeftRadius(int topLeftRadius);
    void resetTopLeftRadius();

    int topRightRadius() const;
    void setTopRightRadius(int topRightRadius);
    void resetTopRightRadius();

    int bottomLeftRadius() const;
    void setBottomLeftRadius(int bottomLeftRadius);
    void resetBottomLeftRadius();

    int bottomRightRadius() const;
    void setBottomRightRadius(int bottomRightRadius);
    void resetBottomRightRadius();

    bool hasBevel() const;
    void setBevel(bool bevel);
    void resetBevel();

    bool hasTopLeftBevel() const;
    void setTopLeftBevel(bool topLeftBevel);
    void resetTopLeftBevel();

    bool hasTopRightBevel() const;
    void setTopRightBevel(bool topRightBevel);
    void resetTopRightBevel();

    bool hasBottomLeftBevel() const;
    void setBottomLeftBevel(bool bottomLeftBevel);
    void resetBottomLeftBevel();

    bool hasBottomRightBevel() const;
    void setBottomRightBevel(bool bottomRightBevel);
    void resetBottomRightBevel();

    QColor strokeColor() const;
    void setStrokeColor(const QColor &color);

    qreal strokeWidth() const;
    void setStrokeWidth(qreal width);

    QColor fillColor() const;
    void setFillColor(const QColor &color);

    QQuickShapePath::FillRule fillRule() const;
    void setFillRule(QQuickShapePath::FillRule fillRule);

    QQuickShapePath::JoinStyle joinStyle() const;
    void setJoinStyle(QQuickShapePath::JoinStyle style);

    int miterLimit() const;
    void setMiterLimit(int limit);

    QQuickShapePath::CapStyle capStyle() const;
    void setCapStyle(QQuickShapePath::CapStyle style);

    QQuickShapePath::StrokeStyle strokeStyle() const;
    void setStrokeStyle(QQuickShapePath::StrokeStyle style);

    qreal dashOffset() const;
    void setDashOffset(qreal offset);

    QList<qreal> dashPattern() const;
    void setDashPattern(const QList<qreal> &array);

    QQuickShapeGradient *fillGradient() const;
    void setFillGradient(QQuickShapeGradient *fillGradient);
    void resetFillGradient();

    enum class BorderMode {
        Inside,
        Middle,
        Outside
    };
    Q_ENUM(BorderMode)
    BorderMode borderMode() const;
    void setBorderMode(BorderMode borderMode);
    void resetBorderMode();

Q_SIGNALS:
    Q_REVISION(6, 11) void drawTopChanged();
    Q_REVISION(6, 11) void drawRightChanged();
    Q_REVISION(6, 11) void drawBottomChanged();
    Q_REVISION(6, 11) void drawLeftChanged();
    void radiusChanged();
    void topLeftRadiusChanged();
    void topRightRadiusChanged();
    void bottomLeftRadiusChanged();
    void bottomRightRadiusChanged();
    void bevelChanged();
    void topLeftBevelChanged();
    void topRightBevelChanged();
    void bottomLeftBevelChanged();
    void bottomRightBevelChanged();
    void shapePathChanged();
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
    void borderModeChanged();

protected:
    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    void updatePolish() override;

    Q_DISABLE_COPY(QQuickRectangleShape)
    Q_DECLARE_PRIVATE(QQuickRectangleShape)
};

QT_END_NAMESPACE

#endif // QQUICKRECTANGLESHAPE_P_H
