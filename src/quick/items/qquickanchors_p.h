/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQUICKANCHORS_P_H
#define QQUICKANCHORS_P_H

#include <qqml.h>

#include <QtCore/QObject>

#include <private/qtquickglobal_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickAnchorsPrivate;
class QQuickAnchorLine;
class Q_QUICK_PRIVATE_EXPORT QQuickAnchors : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickAnchorLine left READ left WRITE setLeft RESET resetLeft NOTIFY leftChanged)
    Q_PROPERTY(QQuickAnchorLine right READ right WRITE setRight RESET resetRight NOTIFY rightChanged)
    Q_PROPERTY(QQuickAnchorLine horizontalCenter READ horizontalCenter WRITE setHorizontalCenter RESET resetHorizontalCenter NOTIFY horizontalCenterChanged)
    Q_PROPERTY(QQuickAnchorLine top READ top WRITE setTop RESET resetTop NOTIFY topChanged)
    Q_PROPERTY(QQuickAnchorLine bottom READ bottom WRITE setBottom RESET resetBottom NOTIFY bottomChanged)
    Q_PROPERTY(QQuickAnchorLine verticalCenter READ verticalCenter WRITE setVerticalCenter RESET resetVerticalCenter NOTIFY verticalCenterChanged)
    Q_PROPERTY(QQuickAnchorLine baseline READ baseline WRITE setBaseline RESET resetBaseline NOTIFY baselineChanged)
    Q_PROPERTY(qreal margins READ margins WRITE setMargins NOTIFY marginsChanged)
    Q_PROPERTY(qreal leftMargin READ leftMargin WRITE setLeftMargin NOTIFY leftMarginChanged)
    Q_PROPERTY(qreal rightMargin READ rightMargin WRITE setRightMargin NOTIFY rightMarginChanged)
    Q_PROPERTY(qreal horizontalCenterOffset READ horizontalCenterOffset WRITE setHorizontalCenterOffset NOTIFY horizontalCenterOffsetChanged)
    Q_PROPERTY(qreal topMargin READ topMargin WRITE setTopMargin NOTIFY topMarginChanged)
    Q_PROPERTY(qreal bottomMargin READ bottomMargin WRITE setBottomMargin NOTIFY bottomMarginChanged)
    Q_PROPERTY(qreal verticalCenterOffset READ verticalCenterOffset WRITE setVerticalCenterOffset NOTIFY verticalCenterOffsetChanged)
    Q_PROPERTY(qreal baselineOffset READ baselineOffset WRITE setBaselineOffset NOTIFY baselineOffsetChanged)
    Q_PROPERTY(QQuickItem *fill READ fill WRITE setFill RESET resetFill NOTIFY fillChanged)
    Q_PROPERTY(QQuickItem *centerIn READ centerIn WRITE setCenterIn RESET resetCenterIn NOTIFY centerInChanged)
    Q_PROPERTY(bool mirrored READ mirrored NOTIFY mirroredChanged)
    Q_PROPERTY(bool alignWhenCentered READ alignWhenCentered WRITE setAlignWhenCentered NOTIFY centerAlignedChanged)

public:
    QQuickAnchors(QQuickItem *item, QObject *parent=0);
    virtual ~QQuickAnchors();

    enum Anchor {
        LeftAnchor = 0x01,
        RightAnchor = 0x02,
        TopAnchor = 0x04,
        BottomAnchor = 0x08,
        HCenterAnchor = 0x10,
        VCenterAnchor = 0x20,
        BaselineAnchor = 0x40,
        Horizontal_Mask = LeftAnchor | RightAnchor | HCenterAnchor,
        Vertical_Mask = TopAnchor | BottomAnchor | VCenterAnchor | BaselineAnchor
    };
    Q_DECLARE_FLAGS(Anchors, Anchor)

    QQuickAnchorLine left() const;
    void setLeft(const QQuickAnchorLine &edge);
    void resetLeft();

    QQuickAnchorLine right() const;
    void setRight(const QQuickAnchorLine &edge);
    void resetRight();

    QQuickAnchorLine horizontalCenter() const;
    void setHorizontalCenter(const QQuickAnchorLine &edge);
    void resetHorizontalCenter();

    QQuickAnchorLine top() const;
    void setTop(const QQuickAnchorLine &edge);
    void resetTop();

    QQuickAnchorLine bottom() const;
    void setBottom(const QQuickAnchorLine &edge);
    void resetBottom();

    QQuickAnchorLine verticalCenter() const;
    void setVerticalCenter(const QQuickAnchorLine &edge);
    void resetVerticalCenter();

    QQuickAnchorLine baseline() const;
    void setBaseline(const QQuickAnchorLine &edge);
    void resetBaseline();

    qreal leftMargin() const;
    void setLeftMargin(qreal);

    qreal rightMargin() const;
    void setRightMargin(qreal);

    qreal horizontalCenterOffset() const;
    void setHorizontalCenterOffset(qreal);

    qreal topMargin() const;
    void setTopMargin(qreal);

    qreal bottomMargin() const;
    void setBottomMargin(qreal);

    qreal margins() const;
    void setMargins(qreal);

    qreal verticalCenterOffset() const;
    void setVerticalCenterOffset(qreal);

    qreal baselineOffset() const;
    void setBaselineOffset(qreal);

    QQuickItem *fill() const;
    void setFill(QQuickItem *);
    void resetFill();

    QQuickItem *centerIn() const;
    void setCenterIn(QQuickItem *);
    void resetCenterIn();

    Anchors usedAnchors() const;

    bool mirrored();

    bool alignWhenCentered() const;
    void setAlignWhenCentered(bool);

    void classBegin();
    void componentComplete();

Q_SIGNALS:
    void leftChanged();
    void rightChanged();
    void topChanged();
    void bottomChanged();
    void verticalCenterChanged();
    void horizontalCenterChanged();
    void baselineChanged();
    void fillChanged();
    void centerInChanged();
    void leftMarginChanged();
    void rightMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void marginsChanged();
    void verticalCenterOffsetChanged();
    void horizontalCenterOffsetChanged();
    void baselineOffsetChanged();
    void mirroredChanged();
    void centerAlignedChanged();

private:
    friend class QQuickItemPrivate;
    Q_DISABLE_COPY(QQuickAnchors)
    Q_DECLARE_PRIVATE(QQuickAnchors)
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QQuickAnchors::Anchors)

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickAnchors)

QT_END_HEADER

#endif // QQUICKANCHORS_P_H
