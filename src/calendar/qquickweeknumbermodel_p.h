/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Calendar module of the Qt Toolkit.
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

#ifndef QQUICKWEEKNUMBERMODEL_P_H
#define QQUICKWEEKNUMBERMODEL_P_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qlocale.h>
#include <QtQuickCalendar/private/qtquickcalendarglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickWeekNumberModelPrivate;

class Q_QUICKCALENDAR_EXPORT QQuickWeekNumberModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(int count READ rowCount CONSTANT FINAL)

public:
    explicit QQuickWeekNumberModel(QObject *parent = Q_NULLPTR);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QLocale locale() const;
    void setLocale(const QLocale &locale);

    Q_INVOKABLE int weekNumberAt(int index) const;
    Q_INVOKABLE int indexOf(int weekNumber) const;

    enum {
        WeekNumberRole = Qt::UserRole + 1
    };

    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void localeChanged();

private:
    Q_DISABLE_COPY(QQuickWeekNumberModel)
    Q_DECLARE_PRIVATE(QQuickWeekNumberModel)
};

QT_END_NAMESPACE

#endif // QQUICKWEEKNUMBERMODEL_P_H
