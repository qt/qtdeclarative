/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#ifndef QQUICKTHEME_P_H
#define QQUICKTHEME_P_H

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

#include <QtQml/qqml.h>
#include <QtGui/qcolor.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQuickThemeData;
class QQuickThemeAttachedPrivate;

class QQuickThemeAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor RESET resetAccentColor NOTIFY accentColorChanged FINAL)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor RESET resetBackgroundColor NOTIFY backgroundColorChanged FINAL)
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor RESET resetBaseColor NOTIFY baseColorChanged FINAL)
    Q_PROPERTY(QColor disabledColor READ disabledColor WRITE setDisabledColor RESET resetDisabledColor NOTIFY disabledColorChanged FINAL)
    Q_PROPERTY(QColor focusColor READ focusColor WRITE setFocusColor RESET resetFocusColor NOTIFY focusColorChanged FINAL)
    Q_PROPERTY(QColor frameColor READ frameColor WRITE setFrameColor RESET resetFrameColor NOTIFY frameColorChanged FINAL)
    Q_PROPERTY(QColor pressColor READ pressColor WRITE setPressColor RESET resetPressColor NOTIFY pressColorChanged FINAL)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor RESET resetSelectedTextColor NOTIFY selectedTextColorChanged FINAL)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor RESET resetSelectionColor NOTIFY selectionColorChanged FINAL)
    Q_PROPERTY(QColor shadowColor READ shadowColor WRITE setShadowColor RESET resetShadowColor NOTIFY shadowColorChanged FINAL)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor RESET resetTextColor NOTIFY textColorChanged FINAL)

public:
    explicit QQuickThemeAttached(const QQuickThemeData &data, QObject *parent = Q_NULLPTR);
    ~QQuickThemeAttached();

    static QQuickThemeAttached *qmlAttachedProperties(QObject *object);

    QQuickThemeAttached *parentTheme() const;
    void setParentTheme(QQuickThemeAttached *theme);

    QColor accentColor() const;
    void setAccentColor(const QColor &color);
    void resetAccentColor();

    QColor disabledColor() const;
    void setDisabledColor(const QColor &color);
    void resetDisabledColor();

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor &color);
    void resetBackgroundColor();

    QColor baseColor() const;
    void setBaseColor(const QColor &color);
    void resetBaseColor();

    QColor focusColor() const;
    void setFocusColor(const QColor &color);
    void resetFocusColor();

    QColor frameColor() const;
    void setFrameColor(const QColor &color);
    void resetFrameColor();

    QColor pressColor() const;
    void setPressColor(const QColor &color);
    void resetPressColor();

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor &color);
    void resetSelectedTextColor();

    QColor selectionColor() const;
    void setSelectionColor(const QColor &color);
    void resetSelectionColor();

    QColor shadowColor() const;
    void setShadowColor(const QColor &color);
    void resetShadowColor();

    QColor textColor() const;
    void setTextColor(const QColor &color);
    void resetTextColor();

Q_SIGNALS:
    void accentColorChanged();
    void backgroundColorChanged();
    void baseColorChanged();
    void disabledColorChanged();
    void focusColorChanged();
    void frameColorChanged();
    void pressColorChanged();
    void selectedTextColorChanged();
    void selectionColorChanged();
    void shadowColorChanged();
    void textColorChanged();

private:
    Q_DISABLE_COPY(QQuickThemeAttached)
    Q_DECLARE_PRIVATE(QQuickThemeAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPEINFO(QQuickThemeAttached, QML_HAS_ATTACHED_PROPERTIES)

#endif // QQUICKTHEME_P_H
