/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmlbench tool.
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

#ifndef TESTMODEL_H
#define TESTMODEL_H

#include <QAbstractListModel>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QJSValue>
#include <QDateTime>
#include <QColor>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QtQml/qqmlregistration.h>

class TestModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    TestModel();
    ~TestModel();

    enum ModelRoles {
        StringRole = Qt::UserRole,
        ByteArrayRole,
        BoolRole,
        IntRole,
        RealRole,
        UrlRole,
        JsValueRole,
        DateTimeRole,
        ColorRole,
        SizeRole,
        PointRole,
        RectRole
    };

    Q_PROPERTY(int rowNumber READ rowNumber WRITE setRowNumber NOTIFY rowNumberChanged)
    int rowNumber() const;
    void setRowNumber(int rowNumber);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex &, int role) const override;

    // TODO: consider testing setData too.

signals:
    void rowNumberChanged();

private:
    int m_rowNumber;
    QHash<int, QByteArray> m_roleNames;

    // model data below here
    QVector<QString> m_strings;
    QVector<QByteArray> m_byteArrays;
    QVector<bool> m_bools;
    QVector<int> m_ints;
    QVector<qreal> m_reals;
    QVector<QUrl> m_urls;
    QVector<QJSValue> m_jsValues;
    QVector<QDateTime> m_dateTimes;
    QVector<QColor> m_colors;
    QVector<QSize> m_sizes;
    QVector<QPoint> m_points;
    QVector<QRect> m_rects;
};

#endif // TESTMODEL_H

