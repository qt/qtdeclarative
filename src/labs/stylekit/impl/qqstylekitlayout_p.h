// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKUNIFIEDLAYOUT_H
#define QQUICKUNIFIEDLAYOUT_H

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


#include <QtQml/QtQml>
#include <QtCore/qtimer.h>
#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class QQStyleKitLayoutItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *item READ item WRITE setItem NOTIFY itemChanged FINAL)
    Q_PROPERTY(qreal x READ x NOTIFY xChanged FINAL)
    Q_PROPERTY(qreal y READ y NOTIFY yChanged FINAL)
    Q_PROPERTY(qreal width READ width NOTIFY widthChanged FINAL)
    Q_PROPERTY(qreal height READ height NOTIFY heightChanged FINAL)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged FINAL)
    Q_PROPERTY(QMarginsF margins READ margins WRITE setMargins NOTIFY marginsChanged FINAL)
    Q_PROPERTY(bool fillWidth READ fillWidth WRITE setFillWidth NOTIFY fillWidthChanged FINAL)
    Q_PROPERTY(bool fillHeight READ fillHeight WRITE setFillHeight NOTIFY fillHeightChanged FINAL)
    QML_NAMED_ELEMENT(StyleKitLayoutItem)

public:
    QQStyleKitLayoutItem(QObject *parent = nullptr);
    QQuickItem *item() const;
    void setItem(QQuickItem *item);

    qreal x() const;
    void setX(qreal x);
    qreal y() const;
    void setY(qreal y);
    qreal width() const;
    void setWidth(qreal width);
    qreal height() const;
    void setHeight(qreal height);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    QMarginsF margins() const;
    void setMargins(const QMarginsF &margins);

    bool fillWidth() const;
    void setFillWidth(bool fill);

    bool fillHeight() const;
    void setFillHeight(bool fill);

signals:
    void itemChanged();
    void xChanged();
    void yChanged();
    void widthChanged();
    void heightChanged();
    void alignmentChanged();
    void marginsChanged();
    void fillWidthChanged();
    void fillHeightChanged();

private:
    QPointer<QQuickItem> m_item;
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    QMarginsF m_margins;
    bool m_fillWidth = false;
    bool m_fillHeight = false;
    qreal m_x = 0;
    qreal m_y = 0;
    qreal m_width = 0;
    qreal m_height = 0;
};

class QQStyleKitLayout : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *container READ container WRITE setContainer NOTIFY containerChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQStyleKitLayoutItem> layoutItems READ layoutItems NOTIFY layoutItemsChanged FINAL)
    Q_PROPERTY(QMarginsF padding READ padding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(QMarginsF contentMargins READ contentMargins WRITE setContentMargins NOTIFY contentMarginsChanged FINAL)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(bool mirrored READ isMirrored WRITE setMirrored NOTIFY mirroredChanged FINAL)
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged FINAL)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged FINAL)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_CLASSINFO("DefaultProperty", "layoutItems")
    QML_NAMED_ELEMENT(StyleKitLayout)

public:
    QQStyleKitLayout(QQuickItem *parent = nullptr);

    QQuickItem *container() const;
    void setContainer(QQuickItem *item);

    QQmlListProperty<QQStyleKitLayoutItem> layoutItems();

    QMarginsF padding() const;

    QMarginsF contentMargins() const;
    void setContentMargins(const QMarginsF &margins);

    qreal spacing() const;
    void setSpacing(qreal spacing);

    bool isMirrored() const;
    void setMirrored(bool mirrored);

    qreal implicitWidth() const;
    qreal implicitHeight() const;

    void setImplicitWidth(qreal width);
    void setImplicitHeight(qreal height);

    bool isEnabled() const;
    void setEnabled(bool enabled);

signals:
    void containerChanged();
    void layoutItemsChanged();
    void layoutChanged();
    void paddingChanged();
    void contentMarginsChanged();
    void spacingChanged();
    void mirroredChanged();
    void implicitWidthChanged();
    void implicitHeightChanged();
    void enabledChanged();

private:
    void updatePolish() override;

    static void layoutItem_append(QQmlListProperty<QQStyleKitLayoutItem> *list, QQStyleKitLayoutItem *item);
    static qsizetype layoutItem_count(QQmlListProperty<QQStyleKitLayoutItem> *list);
    static QQStyleKitLayoutItem *layoutItem_at(QQmlListProperty<QQStyleKitLayoutItem> *list, qsizetype index);
    static void layoutItem_clear(QQmlListProperty<QQStyleKitLayoutItem> *list);

    QPointer<QQuickItem> m_container;
    QList<QQStyleKitLayoutItem *> m_layoutItems;
    QMarginsF m_contentMargins;
    QMarginsF m_padding;
    qreal m_spacing = 0;
    qreal m_implicitWidth = 0;;
    qreal m_implicitHeight = 0;

    bool m_mirrored: 1;
    bool m_enabled: 1;
};

QT_END_NAMESPACE
#endif // QQUICKUNIFIEDLAYOUT_H
