/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquickpage_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Page
    \inherits Control
    \instantiates QQuickPage
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-containers
    \brief A page control.

    Page is a container control which makes it convenient to add
    a \l header and \l footer item to a page.

    \image qtlabscontrols-page-wireframe.png

    The following example snippet illustrates how to use a page-specific
    toolbar header and an application-wide tabbar footer.

    \qml
    import Qt.labs.controls 1.0

    ApplicationWindow {
        visible: true

        StackView {
            anchors.fill: parent

            initialItem: Page {
                header: ToolBar {
                    // ...
                }
            }
        }

        footer: TabBar {
            // ...
        }
    }
    \endqml

    \sa ApplicationWindow, {Container Controls}
*/

class QQuickPagePrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickPage)

public:
    QQuickPagePrivate();

    void relayout();

    void itemImplicitWidthChanged(QQuickItem *item) Q_DECL_OVERRIDE;
    void itemImplicitHeightChanged(QQuickItem *item) Q_DECL_OVERRIDE;

    QQuickItem *header;
    QQuickItem *footer;
};

QQuickPagePrivate::QQuickPagePrivate() : header(Q_NULLPTR), footer(Q_NULLPTR)
{
}

void QQuickPagePrivate::relayout()
{
    Q_Q(QQuickPage);
    QQuickItem *content = q->contentItem();
    const qreal hh = header ? header->height() : 0;
    const qreal fh = footer ? footer->height() : 0;

    content->setY(hh + q->topPadding());
    content->setX(q->leftPadding());
    content->setWidth(q->availableWidth());
    content->setHeight(q->availableHeight() - hh - fh);

    if (header)
        header->setWidth(q->width());

    if (footer) {
        footer->setY(q->height() - fh);
        footer->setWidth(q->width());
    }
}

void QQuickPagePrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickPagePrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

QQuickPage::QQuickPage(QQuickItem *parent) :
    QQuickControl(*(new QQuickPagePrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::AllButtons);
}

/*!
    \qmlproperty Item Qt.labs.controls::Page::header

    This property holds the page header item. The header item is positioned to
    the top, and resized to the width of the page. The default value is \c null.

    \sa footer, ApplicationWindow::header
*/
QQuickItem *QQuickPage::header() const
{
    Q_D(const QQuickPage);
    return d->header;
}

void QQuickPage::setHeader(QQuickItem *header)
{
    Q_D(QQuickPage);
    if (d->header != header) {
        if (d->header) {
            QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            d->header->setParentItem(Q_NULLPTR);
        }
        d->header = header;
        if (header) {
            header->setParentItem(this);
            QQuickItemPrivate *p = QQuickItemPrivate::get(header);
            p->addItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            if (qFuzzyIsNull(header->z()))
                header->setZ(1);
            if (isComponentComplete())
                d->relayout();
        }
        emit headerChanged();
    }
}

/*!
    \qmlproperty Item Qt.labs.controls::Page::footer

    This property holds the page footer item. The footer item is positioned to
    the bottom, and resized to the width of the page. The default value is \c null.

    \sa header, ApplicationWindow::footer
*/
QQuickItem *QQuickPage::footer() const
{
    Q_D(const QQuickPage);
    return d->footer;
}

void QQuickPage::setFooter(QQuickItem *footer)
{
    Q_D(QQuickPage);
    if (d->footer != footer) {
        if (d->footer) {
            QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            d->footer->setParentItem(Q_NULLPTR);
        }
        d->footer = footer;
        if (footer) {
            footer->setParentItem(this);
            QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
            p->addItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            if (qFuzzyIsNull(footer->z()))
                footer->setZ(1);
            if (isComponentComplete())
                d->relayout();
        }
        emit footerChanged();
    }
}

/*!
    \qmlproperty list<Object> Qt.labs.controls::Page::contentData
    \default

    This property holds the list of content data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickPage::contentData()
{
    Q_D(QQuickPage);
    return QQmlListProperty<QObject>(d->contentItem, Q_NULLPTR,
                                     QQuickItemPrivate::data_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlproperty list<Item> Qt.labs.controls::Page::contentChildren

    This property holds the list of content children.

    \sa Item::children
*/
QQmlListProperty<QQuickItem> QQuickPage::contentChildren()
{
    Q_D(QQuickPage);
    return QQmlListProperty<QQuickItem>(d->contentItem, Q_NULLPTR,
                                        QQuickItemPrivate::children_append,
                                        QQuickItemPrivate::children_count,
                                        QQuickItemPrivate::children_at,
                                        QQuickItemPrivate::children_clear);
}

void QQuickPage::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    QQuickControl::contentItemChange(newItem, oldItem);
    if (oldItem)
        disconnect(oldItem, &QQuickItem::childrenChanged, this, &QQuickPage::contentChildrenChanged);
    if (newItem)
        connect(newItem, &QQuickItem::childrenChanged, this, &QQuickPage::contentChildrenChanged);
    emit contentChildrenChanged();
}

void QQuickPage::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickPage);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    d->relayout();
}

void QQuickPage::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickPage);
    QQuickControl::paddingChange(newPadding, oldPadding);
    d->relayout();
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickPage::accessibleRole() const
{
    return QAccessible::PageTab;
}
#endif

QT_END_NAMESPACE
