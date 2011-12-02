/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKITEMVIEW_P_H
#define QQUICKITEMVIEW_P_H

#include "qquickflickable_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeChangeSet;

class QQuickItemViewPrivate;

class Q_AUTOTEST_EXPORT QQuickItemView : public QQuickFlickable
{
    Q_OBJECT

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *currentItem READ currentItem NOTIFY currentItemChanged)

    Q_PROPERTY(bool keyNavigationWraps READ isWrapEnabled WRITE setWrapEnabled NOTIFY keyNavigationWrapsChanged)
    Q_PROPERTY(int cacheBuffer READ cacheBuffer WRITE setCacheBuffer NOTIFY cacheBufferChanged)

    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged)
    Q_PROPERTY(Qt::LayoutDirection effectiveLayoutDirection READ effectiveLayoutDirection NOTIFY effectiveLayoutDirectionChanged)

    Q_PROPERTY(QDeclarativeComponent *header READ header WRITE setHeader NOTIFY headerChanged)
    Q_PROPERTY(QQuickItem *headerItem READ headerItem NOTIFY headerItemChanged)
    Q_PROPERTY(QDeclarativeComponent *footer READ footer WRITE setFooter NOTIFY footerChanged)
    Q_PROPERTY(QQuickItem *footerItem READ footerItem NOTIFY footerItemChanged)

    Q_PROPERTY(QDeclarativeComponent *highlight READ highlight WRITE setHighlight NOTIFY highlightChanged)
    Q_PROPERTY(QQuickItem *highlightItem READ highlightItem NOTIFY highlightItemChanged)
    Q_PROPERTY(bool highlightFollowsCurrentItem READ highlightFollowsCurrentItem WRITE setHighlightFollowsCurrentItem NOTIFY highlightFollowsCurrentItemChanged)
    Q_PROPERTY(HighlightRangeMode highlightRangeMode READ highlightRangeMode WRITE setHighlightRangeMode NOTIFY highlightRangeModeChanged)
    Q_PROPERTY(qreal preferredHighlightBegin READ preferredHighlightBegin WRITE setPreferredHighlightBegin NOTIFY preferredHighlightBeginChanged RESET resetPreferredHighlightBegin)
    Q_PROPERTY(qreal preferredHighlightEnd READ preferredHighlightEnd WRITE setPreferredHighlightEnd NOTIFY preferredHighlightEndChanged RESET resetPreferredHighlightEnd)
    Q_PROPERTY(int highlightMoveDuration READ highlightMoveDuration WRITE setHighlightMoveDuration NOTIFY highlightMoveDurationChanged)

    Q_ENUMS(HighlightRangeMode)
    Q_ENUMS(PositionMode)

public:
    QQuickItemView(QQuickFlickablePrivate &dd, QQuickItem *parent = 0);
    ~QQuickItemView();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    int count() const;

    int currentIndex() const;
    void setCurrentIndex(int idx);

    QQuickItem *currentItem() const;

    bool isWrapEnabled() const;
    void setWrapEnabled(bool);

    int cacheBuffer() const;
    void setCacheBuffer(int);

    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection);
    Qt::LayoutDirection effectiveLayoutDirection() const;

    QDeclarativeComponent *footer() const;
    void setFooter(QDeclarativeComponent *);
    QQuickItem *footerItem() const;

    QDeclarativeComponent *header() const;
    void setHeader(QDeclarativeComponent *);
    QQuickItem *headerItem() const;

    QDeclarativeComponent *highlight() const;
    void setHighlight(QDeclarativeComponent *);

    QQuickItem *highlightItem() const;

    bool highlightFollowsCurrentItem() const;
    virtual void setHighlightFollowsCurrentItem(bool);

    enum HighlightRangeMode { NoHighlightRange, ApplyRange, StrictlyEnforceRange };
    HighlightRangeMode highlightRangeMode() const;
    void setHighlightRangeMode(HighlightRangeMode mode);

    qreal preferredHighlightBegin() const;
    void setPreferredHighlightBegin(qreal);
    void resetPreferredHighlightBegin();

    qreal preferredHighlightEnd() const;
    void setPreferredHighlightEnd(qreal);
    void resetPreferredHighlightEnd();

    int highlightMoveDuration() const;
    virtual void setHighlightMoveDuration(int);

    enum PositionMode { Beginning, Center, End, Visible, Contain };

    Q_INVOKABLE void positionViewAtIndex(int index, int mode);
    Q_INVOKABLE int indexAt(qreal x, qreal y) const;
    Q_INVOKABLE void positionViewAtBeginning();
    Q_INVOKABLE void positionViewAtEnd();

    virtual void setContentX(qreal pos);
    virtual void setContentY(qreal pos);
    virtual qreal xOrigin() const;

signals:
    void modelChanged();
    void delegateChanged();
    void countChanged();
    void currentIndexChanged();
    void currentItemChanged();

    void keyNavigationWrapsChanged();
    void cacheBufferChanged();

    void layoutDirectionChanged();
    void effectiveLayoutDirectionChanged();

    void headerChanged();
    void footerChanged();
    void headerItemChanged();
    void footerItemChanged();

    void highlightChanged();
    void highlightItemChanged();
    void highlightFollowsCurrentItemChanged();
    void highlightRangeModeChanged();
    void preferredHighlightBeginChanged();
    void preferredHighlightEndChanged();
    void highlightMoveDurationChanged();

protected:
    virtual void updatePolish();
    virtual void componentComplete();
    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    virtual qreal minYExtent() const;
    virtual qreal maxYExtent() const;
    virtual qreal minXExtent() const;
    virtual qreal maxXExtent() const;

protected slots:
    virtual void updateSections() {}
    void destroyRemoved();
    void createdItem(int index, QQuickItem *item);
    void initItem(int index, QQuickItem *item);
    void modelUpdated(const QDeclarativeChangeSet &changeSet, bool reset);
    void destroyingItem(QQuickItem *item);
    void animStopped();
    void trackedPositionChanged();



private:
    Q_DECLARE_PRIVATE(QQuickItemView)
};


class Q_AUTOTEST_EXPORT QQuickItemViewAttached : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY currentItemChanged)
    Q_PROPERTY(bool delayRemove READ delayRemove WRITE setDelayRemove NOTIFY delayRemoveChanged)

    Q_PROPERTY(QString section READ section NOTIFY sectionChanged)
    Q_PROPERTY(QString previousSection READ prevSection NOTIFY prevSectionChanged)
    Q_PROPERTY(QString nextSection READ nextSection NOTIFY nextSectionChanged)

public:
    QQuickItemViewAttached(QObject *parent)
        : QObject(parent), m_isCurrent(false), m_delayRemove(false) {}
    ~QQuickItemViewAttached() {}

    bool isCurrentItem() const { return m_isCurrent; }
    void setIsCurrentItem(bool c) {
        if (m_isCurrent != c) {
            m_isCurrent = c;
            emit currentItemChanged();
        }
    }

    bool delayRemove() const { return m_delayRemove; }
    void setDelayRemove(bool delay) {
        if (m_delayRemove != delay) {
            m_delayRemove = delay;
            emit delayRemoveChanged();
        }
    }

    QString section() const { return m_section; }
    void setSection(const QString &sect) {
        if (m_section != sect) {
            m_section = sect;
            emit sectionChanged();
        }
    }

    QString prevSection() const { return m_prevSection; }
    void setPrevSection(const QString &sect) {
        if (m_prevSection != sect) {
            m_prevSection = sect;
            emit prevSectionChanged();
        }
    }

    QString nextSection() const { return m_nextSection; }
    void setNextSection(const QString &sect) {
        if (m_nextSection != sect) {
            m_nextSection = sect;
            emit nextSectionChanged();
        }
    }

    void emitAdd() { emit add(); }
    void emitRemove() { emit remove(); }

signals:
    void currentItemChanged();
    void delayRemoveChanged();

    void add();
    void remove();

    void sectionChanged();
    void prevSectionChanged();
    void nextSectionChanged();

public:
    bool m_isCurrent : 1;
    bool m_delayRemove : 1;

    // current only used by list view
    mutable QString m_section;
    QString m_prevSection;
    QString m_nextSection;
};


QT_END_NAMESPACE
QT_END_HEADER

#endif // QQUICKITEMVIEW_P_H

