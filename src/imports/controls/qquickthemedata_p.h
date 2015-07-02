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

#ifndef QQUICKTHEMEDATA_P_H
#define QQUICKTHEMEDATA_P_H

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

#include <QtGui/qcolor.h>
#include <QtCore/qstring.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QQuickThemeData
{
public:
    QQuickThemeData(const QString &filePath = QString());

    bool load(const QString &filePath);

    QColor accentColor() const { return d->accentColor; }
    void setAccentColor(const QColor &color) { d->accentColor = color; }

    QColor backgroundColor() const { return d->backgroundColor; }
    void setBackgroundColor(const QColor &color) { d->backgroundColor = color; }

    QColor baseColor() const { return d->baseColor; }
    void setBaseColor(const QColor &color) { d->baseColor = color; }

    QColor disabledColor() const { return d->disabledColor; }
    void setDisabledColor(const QColor &color) { d->disabledColor = color; }

    QColor focusColor() const { return d->focusColor; }
    void setFocusColor(const QColor &color) { d->focusColor = color; }

    QColor frameColor() const { return d->frameColor; }
    void setFrameColor(const QColor &color) { d->frameColor = color; }

    QColor pressColor() const { return d->pressColor; }
    void setPressColor(const QColor &color) { d->pressColor = color; }

    QColor selectedTextColor() const { return d->selectedTextColor; }
    void setSelectedTextColor(const QColor &color) { d->selectedTextColor = color; }

    QColor selectionColor() const { return d->selectionColor; }
    void setSelectionColor(const QColor &color) { d->selectionColor = color; }

    QColor shadowColor() const { return d->shadowColor; }
    void setShadowColor(const QColor &color) { d->shadowColor = color; }

    QColor textColor() const { return d->textColor; }
    void setTextColor(const QColor &color) { d->textColor = color; }

private:
    struct Data : public QSharedData {
        QColor accentColor;
        QColor baseColor;
        QColor backgroundColor;
        QColor disabledColor;
        QColor focusColor;
        QColor frameColor;
        QColor pressColor;
        QColor selectedTextColor;
        QColor selectionColor;
        QColor shadowColor;
        QColor textColor;
    };
    QSharedDataPointer<Data> d;
};

QT_END_NAMESPACE

#endif // QQUICKTHEMEDATA_P_H
