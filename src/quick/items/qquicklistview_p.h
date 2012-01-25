// Commit: 95814418f9d6adeba365c795462e8afb00138211
/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKLISTVIEW_P_H
#define QQUICKLISTVIEW_P_H

#include "qquickitemview_p.h"

#include <private/qdeclarativeguard_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickListView;
class QQuickListViewPrivate;
class Q_AUTOTEST_EXPORT QQuickViewSection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(SectionCriteria criteria READ criteria WRITE setCriteria NOTIFY criteriaChanged)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int labelPositioning READ labelPositioning WRITE setLabelPositioning NOTIFY labelPositioningChanged)
    Q_ENUMS(SectionCriteria)
    Q_ENUMS(LabelPositioning)
public:
    QQuickViewSection(QQuickListView *parent=0);

    QString property() const { return m_property; }
    void setProperty(const QString &);

    enum SectionCriteria { FullString, FirstCharacter };
    SectionCriteria criteria() const { return m_criteria; }
    void setCriteria(SectionCriteria);

    QDeclarativeComponent *delegate() const { return m_delegate; }
    void setDelegate(QDeclarativeComponent *delegate);

    QString sectionString(const QString &value);

    enum LabelPositioning { InlineLabels = 0x01, CurrentLabelAtStart = 0x02, NextLabelAtEnd = 0x04 };
    int labelPositioning() { return m_labelPositioning; }
    void setLabelPositioning(int pos);

Q_SIGNALS:
    void propertyChanged();
    void criteriaChanged();
    void delegateChanged();
    void labelPositioningChanged();

private:
    QString m_property;
    SectionCriteria m_criteria;
    QDeclarativeComponent *m_delegate;
    int m_labelPositioning;
    QQuickListViewPrivate *m_view;
};


class QQuickVisualModel;
class QQuickListViewAttached;
class Q_AUTOTEST_EXPORT QQuickListView : public QQuickItemView
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickListView)

    // XXX deprecate these two properties (only duration should be necessary)
    Q_PROPERTY(qreal highlightMoveSpeed READ highlightMoveSpeed WRITE setHighlightMoveSpeed NOTIFY highlightMoveSpeedChanged)
    Q_PROPERTY(qreal highlightResizeSpeed READ highlightResizeSpeed WRITE setHighlightResizeSpeed NOTIFY highlightResizeSpeedChanged)

    Q_PROPERTY(int highlightResizeDuration READ highlightResizeDuration WRITE setHighlightResizeDuration NOTIFY highlightResizeDurationChanged)

    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)

    Q_PROPERTY(QQuickViewSection *section READ sectionCriteria CONSTANT)
    Q_PROPERTY(QString currentSection READ currentSection NOTIFY currentSectionChanged)

    Q_PROPERTY(SnapMode snapMode READ snapMode WRITE setSnapMode NOTIFY snapModeChanged)

    Q_ENUMS(Orientation)
    Q_ENUMS(SnapMode)
    Q_CLASSINFO("DefaultProperty", "data")

public:
    QQuickListView(QQuickItem *parent=0);
    ~QQuickListView();

    qreal spacing() const;
    void setSpacing(qreal spacing);

    enum Orientation { Horizontal = Qt::Horizontal, Vertical = Qt::Vertical };
    Orientation orientation() const;
    void setOrientation(Orientation);

    QQuickViewSection *sectionCriteria();
    QString currentSection() const;

    virtual void setHighlightFollowsCurrentItem(bool);

    qreal highlightMoveSpeed() const;
    void setHighlightMoveSpeed(qreal);

    qreal highlightResizeSpeed() const;
    void setHighlightResizeSpeed(qreal);

    int highlightResizeDuration() const;
    void setHighlightResizeDuration(int);

    virtual void setHighlightMoveDuration(int);

    enum SnapMode { NoSnap, SnapToItem, SnapOneItem };
    SnapMode snapMode() const;
    void setSnapMode(SnapMode mode);

    static QQuickListViewAttached *qmlAttachedProperties(QObject *);

public Q_SLOTS:
    void incrementCurrentIndex();
    void decrementCurrentIndex();

Q_SIGNALS:
    void spacingChanged();
    void orientationChanged();
    void currentSectionChanged();
    void highlightMoveSpeedChanged();
    void highlightResizeSpeedChanged();
    void highlightResizeDurationChanged();
    void snapModeChanged();

protected:
    virtual void viewportMoved();
    virtual void keyPressEvent(QKeyEvent *);
    virtual void geometryChanged(const QRectF &newGeometry,const QRectF &oldGeometry);

protected Q_SLOTS:
    void updateSections();
};

class QQuickListViewAttached : public QQuickItemViewAttached
{
    Q_OBJECT

public:
    QQuickListViewAttached(QObject *parent)
        : QQuickItemViewAttached(parent), m_view(0) {}
    ~QQuickListViewAttached() {}

    Q_PROPERTY(QQuickListView *view READ view NOTIFY viewChanged)
    QQuickListView *view() { return m_view; }
    void setView(QQuickListView *view) {
        if (view != m_view) {
            m_view = view;
            emit viewChanged();
        }
    }

Q_SIGNALS:
    void viewChanged();

public:
    QDeclarativeGuard<QQuickListView> m_view;
};


QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickListView, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QQuickListView)
QML_DECLARE_TYPE(QQuickViewSection)

QT_END_HEADER

#endif // QQUICKLISTVIEW_P_H
