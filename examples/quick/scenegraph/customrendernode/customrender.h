// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CUSTOMRENDER_H
#define CUSTOMRENDER_H

#include <QQuickItem>
#include <QVector2D>

//![item]
class CustomRender : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QList<QVector2D> vertices READ vertices WRITE setVertices NOTIFY verticesChanged)
    QML_ELEMENT

public:
    explicit CustomRender(QQuickItem *parent = nullptr);

    QList<QVector2D> vertices() const;
    void setVertices(const QList<QVector2D> &newVertices);

signals:
    void verticesChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *) override;

private:
    QList<QVector2D> m_vertices;
};
//![item]

#endif // CUSTOMRENDER_H
