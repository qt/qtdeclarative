// Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef XORBLENDER_H
#define XORBLENDER_H

#include <QQuickItem>

class XorBlender : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQuickItem *source1 READ source1 WRITE setSource1 NOTIFY source1Changed)
    Q_PROPERTY(QQuickItem *source2 READ source2 WRITE setSource2 NOTIFY source2Changed)
    QML_ELEMENT

public:
    explicit XorBlender(QQuickItem *parent = nullptr);

    QQuickItem *source1() const { return m_source1; }
    QQuickItem *source2() const { return m_source2; }

    void setSource1(QQuickItem *i);
    void setSource2(QQuickItem *i);

protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *);

signals:
    void source1Changed(QQuickItem *item);
    void source2Changed(QQuickItem *item);

private:
    QQuickItem *m_source1;
    QQuickItem *m_source2;

    bool m_source1Changed;
    bool m_source2Changed;
};

#endif // XORBLENDER_H
