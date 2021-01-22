/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQUICKSTACKLAYOUT_H
#define QQUICKSTACKLAYOUT_H

#include <qquicklayout_p.h>

QT_BEGIN_NAMESPACE

class QQuickStackLayoutPrivate;

class QQuickStackLayout : public QQuickLayout
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    QML_NAMED_ELEMENT(StackLayout)
    QML_ADDED_IN_MINOR_VERSION(3)

public:
    explicit QQuickStackLayout(QQuickItem *parent = 0);
    int count() const;
    int currentIndex() const;
    void setCurrentIndex(int index);

    void componentComplete() override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;
    QSizeF sizeHint(Qt::SizeHint whichSizeHint) const override;
    void setAlignment(QQuickItem *item, Qt::Alignment align)  override;
    void invalidate(QQuickItem *childItem = 0)  override;
    void updateLayoutItems()  override;
    void rearrange(const QSizeF &) override;

    // iterator
    Q_INVOKABLE QQuickItem *itemAt(int index) const override;
    int itemCount() const override;
    int indexOf(QQuickItem *item) const;



signals:
    void currentIndexChanged();
    void countChanged();

public slots:

private:
    static void collectItemSizeHints(QQuickItem *item, QSizeF *sizeHints);
    bool shouldIgnoreItem(QQuickItem *item) const;
    Q_DECLARE_PRIVATE(QQuickStackLayout)

    QList<QQuickItem*> m_items;

    typedef struct {
        inline QSizeF &min() { return array[Qt::MinimumSize]; }
        inline QSizeF &pref() { return array[Qt::PreferredSize]; }
        inline QSizeF &max() { return array[Qt::MaximumSize]; }
        QSizeF array[Qt::NSizeHints];
    } SizeHints;

    mutable QVector<SizeHints> m_cachedItemSizeHints;
    mutable QSizeF m_cachedSizeHints[Qt::NSizeHints];
};

class QQuickStackLayoutPrivate : public QQuickLayoutPrivate
{
    Q_DECLARE_PUBLIC(QQuickStackLayout)
public:
    QQuickStackLayoutPrivate() : count(0), currentIndex(-1), explicitCurrentIndex(false) {}
private:
    int count;
    int currentIndex;
    bool explicitCurrentIndex;
};

QT_END_NAMESPACE

#endif // QQUICKSTACKLAYOUT_H
