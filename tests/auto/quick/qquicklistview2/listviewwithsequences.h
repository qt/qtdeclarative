/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef LISTVIEWWITHSEQUENCES_H
#define LISTVIEWWITHSEQUENCES_H

#include <private/qquicklistview_p.h>
#include <QtQml/qqml.h>
#include <QtCore/qrect.h>

class ListViewWithSequences : public QQuickListView {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<QRectF> rects READ rects WRITE setRects NOTIFY rectsChanged)
    Q_PROPERTY(QList<QString> texts READ texts WRITE setTexts NOTIFY textsChanged)
public:
    ListViewWithSequences(QQuickItem *parent = nullptr) : QQuickListView(parent) {}

    const QList<QRectF> &rects() const { return m_rects; }
    void setRects(const QList<QRectF> &rects)
    {
        if (rects != m_rects) {
            m_rects = rects;
            emit rectsChanged();
        }
    }

    const QList<QString> &texts() const { return m_texts; }
    void setTexts(const QList<QString> &texts)
    {
        if (texts != m_texts) {
            m_texts = texts;
            emit textsChanged();
        }
    }

signals:
    void rectsChanged();
    void textsChanged();

private:
    QList<QRectF> m_rects;
    QList<QString> m_texts;
};

struct RectListForeign
{
    Q_GADGET
    QML_FOREIGN(QList<QRectF>)
    QML_SEQUENTIAL_CONTAINER(QRectF)
    QML_ANONYMOUS
};

#endif // LISTVIEWWITHSEQUENCES_H
