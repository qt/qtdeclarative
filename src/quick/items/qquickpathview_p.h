// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
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

#ifndef QQUICKPATHVIEW_P_H
#define QQUICKPATHVIEW_P_H

#include "qquickitem.h"

#include <private/qdeclarativepath_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeChangeSet;

class QQuickPathViewPrivate;
class QQuickPathViewAttached;
class Q_AUTOTEST_EXPORT QQuickPathView : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QDeclarativePath *path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged)
    Q_PROPERTY(qreal offset READ offset WRITE setOffset NOTIFY offsetChanged)

    Q_PROPERTY(QDeclarativeComponent *highlight READ highlight WRITE setHighlight NOTIFY highlightChanged)
    Q_PROPERTY(QQuickItem *highlightItem READ highlightItem NOTIFY highlightItemChanged)

    Q_PROPERTY(qreal preferredHighlightBegin READ preferredHighlightBegin WRITE setPreferredHighlightBegin NOTIFY preferredHighlightBeginChanged)
    Q_PROPERTY(qreal preferredHighlightEnd READ preferredHighlightEnd WRITE setPreferredHighlightEnd NOTIFY preferredHighlightEndChanged)
    Q_PROPERTY(HighlightRangeMode highlightRangeMode READ highlightRangeMode WRITE setHighlightRangeMode NOTIFY highlightRangeModeChanged)
    Q_PROPERTY(int highlightMoveDuration READ highlightMoveDuration WRITE setHighlightMoveDuration NOTIFY highlightMoveDurationChanged)

    Q_PROPERTY(qreal dragMargin READ dragMargin WRITE setDragMargin NOTIFY dragMarginChanged)
    Q_PROPERTY(qreal flickDeceleration READ flickDeceleration WRITE setFlickDeceleration NOTIFY flickDecelerationChanged)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged)

    Q_PROPERTY(bool moving READ isMoving NOTIFY movingChanged)
    Q_PROPERTY(bool flicking READ isFlicking NOTIFY flickingChanged)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int pathItemCount READ pathItemCount WRITE setPathItemCount NOTIFY pathItemCountChanged)

    Q_ENUMS(HighlightRangeMode)

public:
    QQuickPathView(QQuickItem *parent=0);
    virtual ~QQuickPathView();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativePath *path() const;
    void setPath(QDeclarativePath *);

    int currentIndex() const;
    void setCurrentIndex(int idx);

    QQuickItem *currentItem() const;

    qreal offset() const;
    void setOffset(qreal offset);

    QDeclarativeComponent *highlight() const;
    void setHighlight(QDeclarativeComponent *highlight);
    QQuickItem *highlightItem();

    enum HighlightRangeMode { NoHighlightRange, ApplyRange, StrictlyEnforceRange };
    HighlightRangeMode highlightRangeMode() const;
    void setHighlightRangeMode(HighlightRangeMode mode);

    qreal preferredHighlightBegin() const;
    void setPreferredHighlightBegin(qreal);

    qreal preferredHighlightEnd() const;
    void setPreferredHighlightEnd(qreal);

    int highlightMoveDuration() const;
    void setHighlightMoveDuration(int);

    qreal dragMargin() const;
    void setDragMargin(qreal margin);

    qreal flickDeceleration() const;
    void setFlickDeceleration(qreal dec);

    bool isInteractive() const;
    void setInteractive(bool);

    bool isMoving() const;
    bool isFlicking() const;

    int count() const;

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    int pathItemCount() const;
    void setPathItemCount(int);

    static QQuickPathViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void incrementCurrentIndex();
    void decrementCurrentIndex();

Q_SIGNALS:
    void currentIndexChanged();
    void currentItemChanged();
    void offsetChanged();
    void modelChanged();
    void countChanged();
    void pathChanged();
    void preferredHighlightBeginChanged();
    void preferredHighlightEndChanged();
    void highlightRangeModeChanged();
    void dragMarginChanged();
    void snapPositionChanged();
    void delegateChanged();
    void pathItemCountChanged();
    void flickDecelerationChanged();
    void interactiveChanged();
    void movingChanged();
    void flickingChanged();
    void highlightChanged();
    void highlightItemChanged();
    void highlightMoveDurationChanged();
    void movementStarted();
    void movementEnded();
    void flickStarted();
    void flickEnded();

protected:
    virtual void updatePolish();
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    bool sendMouseEvent(QMouseEvent *event);
    bool childMouseEventFilter(QQuickItem *, QEvent *);
    void mouseUngrabEvent();
    void componentComplete();

private Q_SLOTS:
    void refill();
    void ticked();
    void movementEnding();
    void modelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);
    void createdItem(int index, QQuickItem *item);
    void initItem(int index, QQuickItem *item);
    void destroyingItem(QQuickItem *item);
    void pathUpdated();

private:
    friend class QQuickPathViewAttached;
    Q_DISABLE_COPY(QQuickPathView)
    Q_DECLARE_PRIVATE(QQuickPathView)
};

class QDeclarativeOpenMetaObject;
class QQuickPathViewAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQuickPathView *view READ view CONSTANT)
    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY currentItemChanged)
    Q_PROPERTY(bool onPath READ isOnPath NOTIFY pathChanged)

public:
    QQuickPathViewAttached(QObject *parent);
    ~QQuickPathViewAttached();

    QQuickPathView *view() { return m_view; }

    bool isCurrentItem() const { return m_isCurrent; }
    void setIsCurrentItem(bool c) {
        if (m_isCurrent != c) {
            m_isCurrent = c;
            emit currentItemChanged();
        }
    }

    QVariant value(const QByteArray &name) const;
    void setValue(const QByteArray &name, const QVariant &val);

    bool isOnPath() const { return m_onPath; }
    void setOnPath(bool on) {
        if (on != m_onPath) {
            m_onPath = on;
            emit pathChanged();
        }
    }
    qreal m_percent;

Q_SIGNALS:
    void currentItemChanged();
    void pathChanged();

private:
    friend class QQuickPathViewPrivate;
    friend class QQuickPathView;
    QQuickPathView *m_view;
    QDeclarativeOpenMetaObject *m_metaobject;
    bool m_onPath : 1;
    bool m_isCurrent : 1;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPathView)
QML_DECLARE_TYPEINFO(QQuickPathView, QML_HAS_ATTACHED_PROPERTIES)
QT_END_HEADER

#endif // QQUICKPATHVIEW_P_H
