/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKINPUTMETHOD_P_H
#define QQUICKINPUTMETHOD_P_H

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

#include <QtCore/qobject.h>
#include <QtCore/qlocale.h>
#include <QtCore/qrect.h>
#include <QtGui/qtransform.h>
#include <QtGui/qinputmethod.h>
#include <QtQml/qqml.h>

#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE
class Q_QUICK_PRIVATE_EXPORT QQuickInputMethod : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethod)
    QML_ADDED_IN_VERSION(6, 4)
    QML_SINGLETON

    Q_PROPERTY(QRectF cursorRectangle READ cursorRectangle NOTIFY cursorRectangleChanged)
    Q_PROPERTY(QRectF anchorRectangle READ anchorRectangle NOTIFY anchorRectangleChanged)
    Q_PROPERTY(QRectF keyboardRectangle READ keyboardRectangle NOTIFY keyboardRectangleChanged)
    Q_PROPERTY(QRectF inputItemClipRectangle READ inputItemClipRectangle NOTIFY
                       inputItemClipRectangleChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool animating READ isAnimating NOTIFY animatingChanged)
    Q_PROPERTY(QLocale locale READ locale NOTIFY localeChanged)
    Q_PROPERTY(Qt::LayoutDirection inputDirection READ inputDirection NOTIFY inputDirectionChanged)
public:
    explicit QQuickInputMethod(QObject *parent = nullptr);

    QRectF anchorRectangle() const;
    QRectF cursorRectangle() const;
    Qt::LayoutDirection inputDirection() const;
    QRectF inputItemClipRectangle() const;

    QRectF inputItemRectangle() const;
    void setInputItemRectangle(const QRectF &rect);

    QTransform inputItemTransform() const;
    void setInputItemTransform(const QTransform &transform);

    bool isAnimating() const;

    bool isVisible() const;
    void setVisible(bool visible);

    QRectF keyboardRectangle() const;
    QLocale locale() const;
signals:
    void anchorRectangleChanged();
    void animatingChanged();
    void cursorRectangleChanged();
    void inputDirectionChanged(Qt::LayoutDirection newDirection);
    void inputItemClipRectangleChanged();
    void keyboardRectangleChanged();
    void localeChanged();
    void visibleChanged();

public slots:
    void commit();
    void hide();
    void invokeAction(QInputMethod::Action a, int cursorPosition);
    void reset();
    void show();
    void update(Qt::InputMethodQueries queries);
};

QT_END_NAMESPACE

#endif // QQUICKINPUTMETHOD_P_H
