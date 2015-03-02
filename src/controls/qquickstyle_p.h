/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#ifndef QQUICKSTYLE_P_H
#define QQUICKSTYLE_P_H

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
#include <QtGui/qcolor.h>
#include <QtQuickControls/private/qtquickcontrolsglobal_p.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QJSEngine;

class Q_QUICKCONTROLS_EXPORT QQuickStyle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor accentColor MEMBER accentColor NOTIFY accentColorChanged FINAL)
    Q_PROPERTY(QColor backgroundColor MEMBER backgroundColor NOTIFY backgroundColorChanged FINAL)
    Q_PROPERTY(QColor baseColor MEMBER baseColor NOTIFY baseColorChanged FINAL)
    Q_PROPERTY(QColor focusColor MEMBER focusColor NOTIFY focusColorChanged FINAL)
    Q_PROPERTY(QColor frameColor MEMBER frameColor NOTIFY frameColorChanged FINAL)
    Q_PROPERTY(QColor pressColor MEMBER pressColor NOTIFY pressColorChanged FINAL)
    Q_PROPERTY(QColor selectedTextColor MEMBER selectedTextColor NOTIFY selectedTextColorChanged FINAL)
    Q_PROPERTY(QColor selectionColor MEMBER selectionColor NOTIFY selectionColorChanged FINAL)
    Q_PROPERTY(QColor shadowColor MEMBER shadowColor NOTIFY shadowColorChanged FINAL)
    Q_PROPERTY(QColor textColor MEMBER textColor NOTIFY textColorChanged FINAL)
    Q_PROPERTY(int padding MEMBER padding NOTIFY paddingChanged FINAL)
    Q_PROPERTY(int roundness MEMBER roundness NOTIFY roundnessChanged FINAL)
    Q_PROPERTY(int spacing MEMBER spacing NOTIFY spacingChanged FINAL)
    Q_PROPERTY(qreal disabledOpacity MEMBER disabledOpacity NOTIFY disabledOpacityChanged FINAL)

public:
    explicit QQuickStyle(QObject *parent = Q_NULLPTR);

    static QQuickStyle *instance(QQmlEngine *engine);
    static QQuickStyle *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void accentColorChanged();
    void backgroundColorChanged();
    void baseColorChanged();
    void focusColorChanged();
    void frameColorChanged();
    void pressColorChanged();
    void selectedTextColorChanged();
    void selectionColorChanged();
    void shadowColorChanged();
    void textColorChanged();
    void paddingChanged();
    void roundnessChanged();
    void spacingChanged();
    void disabledOpacityChanged();

private:
    QColor accentColor;
    QColor baseColor;
    QColor backgroundColor;
    QColor focusColor;
    QColor frameColor;
    QColor pressColor;
    QColor selectedTextColor;
    QColor selectionColor;
    QColor shadowColor;
    QColor textColor;
    int padding;
    int spacing;
    int roundness;
    qreal disabledOpacity;
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickStyle, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKSTYLE_P_H
