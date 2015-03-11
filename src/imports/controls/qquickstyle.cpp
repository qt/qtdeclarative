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

#include "qquickstyle_p.h"
#include "qquickstyledata_p.h"

#include <QtCore/qset.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Style
    \inherits QtObject
    \instantiates QQuickStyle
    \inqmlmodule QtQuick.Controls
    \ingroup utilities
    \brief A style interface.

    TODO
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::accentColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::backgroundColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::focusColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::frameColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::pressColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::selectedTextColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::selectionColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::shadowColor
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Style::textColor
*/

/*!
    \qmlattachedproperty int QtQuickControls2::Style::padding
*/

/*!
    \qmlattachedproperty int QtQuickControls2::Style::roundness
*/

/*!
    \qmlattachedproperty int QtQuickControls2::Style::spacing
*/

/*!
    \qmlattachedproperty real QtQuickControls2::Style::disabledOpacity
*/

Q_GLOBAL_STATIC_WITH_ARGS(QQuickStyleData, globalStyleData, (QString::fromLatin1(":/qtquickcontrols/style.json")))

static QQuickStyle *styleInstance(QQmlEngine *engine)
{
    static QHash<QQmlEngine *, QQuickStyle *> styles;
    QHash<QQmlEngine *, QQuickStyle *>::iterator it = styles.find(engine);
    if (it == styles.end())
        it = styles.insert(engine, new QQuickStyle(*globalStyleData(), engine));
    return it.value();
}

static QQuickStyle *attachedStyle(QObject *object)
{
    if (object)
        return qobject_cast<QQuickStyle*>(qmlAttachedPropertiesObject<QQuickStyle>(object, false));
    return Q_NULLPTR;
}

static QQuickStyle *findParentStyle(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item) {
        // lookup parent items
        QQuickItem *parent = item->parentItem();
        while (parent) {
            QQuickStyle *attached = attachedStyle(parent);
            if (attached)
                return attached;
            parent = parent->parentItem();
        }

        // fallback to item's window style
        QQuickWindow *window = item->window();
        if (window) {
            QQuickStyle *attached = attachedStyle(window);
            if (attached)
                return attached;
        }
    }

    // lookup parent window style
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
    if (window) {
        QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->parent());
        if (parentWindow) {
            QQuickStyle *attached = attachedStyle(window);
            if (attached)
                return attached;
        }
    }

    // fallback to global style
    if (object) {
        QQmlEngine *engine = qmlEngine(object);
        if (engine)
            return styleInstance(engine);
    }

    return Q_NULLPTR;
}

static QList<QQuickStyle *> findChildStyles(QObject *object)
{
    QList<QQuickStyle *> styles;

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window) {
            item = window->contentItem();

            foreach (QObject *child, window->children()) {
                QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
                if (childWindow) {
                    QQuickStyle *style = attachedStyle(childWindow);
                    if (style)
                        styles += style;
                }
            }
        }
    }

    if (item) {
        foreach (QQuickItem *child, item->childItems()) {
            QQuickStyle *style = attachedStyle(child);
            if (style)
                styles += style;
            else
                styles += findChildStyles(child);
        }
    }

    return styles;
}

class QQuickStylePrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickStyle)

public:
    QQuickStylePrivate(const QQuickStyleData &data) : data(data),
        explicitAccentColor(false),
        explicitBackgroundColor(false),
        explicitBaseColor(false),
        explicitFocusColor(false),
        explicitFrameColor(false),
        explicitPressColor(false),
        explicitSelectedTextColor(false),
        explicitSelectionColor(false),
        explicitShadowColor(false),
        explicitTextColor(false),
        explicitPadding(false),
        explicitSpacing(false),
        explicitRoundness(false),
        explicitDisabledOpacity(false) { }

    enum Method { Implicit, Explicit, Inherit };

    void setAccentColor(const QColor &color, Method method);
    void setBackgroundColor(const QColor &color, Method method);
    void setBaseColor(const QColor &color, Method method);
    void setFocusColor(const QColor &color, Method method);
    void setFrameColor(const QColor &color, Method method);
    void setPressColor(const QColor &color, Method method);
    void setSelectedTextColor(const QColor &color, Method method);
    void setSelectionColor(const QColor &color, Method method);
    void setShadowColor(const QColor &color, Method method);
    void setTextColor(const QColor &color, Method method);
    void setPadding(int padding, Method method);
    void setRoundness(int roundness, Method method);
    void setSpacing(int spacing, Method method);
    void setDisabledOpacity(qreal opacity, Method method);

    void inherit(QQuickStyle *style);

    const QQuickStyleData &resolve() const;

    // TODO: add QQuickItemChangeListener::itemSceneChanged()
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) Q_DECL_OVERRIDE;

    QQuickStyleData data;
    QPointer<QQuickStyle> parentStyle;
    QSet<QQuickStyle *> childStyles;

    bool explicitAccentColor;
    bool explicitBackgroundColor;
    bool explicitBaseColor;
    bool explicitFocusColor;
    bool explicitFrameColor;
    bool explicitPressColor;
    bool explicitSelectedTextColor;
    bool explicitSelectionColor;
    bool explicitShadowColor;
    bool explicitTextColor;
    bool explicitPadding;
    bool explicitSpacing;
    bool explicitRoundness;
    bool explicitDisabledOpacity;
};

void QQuickStylePrivate::setAccentColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitAccentColor || method != Inherit) {
        explicitAccentColor = method == Explicit;
        if (data.accentColor() != color) {
            data.setAccentColor(color);
            emit q->accentColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setAccentColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setBackgroundColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitBackgroundColor || method != Inherit) {
        explicitBackgroundColor = method == Explicit;
        if (data.backgroundColor() != color) {
            data.setBackgroundColor(color);
            emit q->backgroundColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setBackgroundColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setBaseColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitBaseColor || method != Inherit) {
        explicitBaseColor = method == Explicit;
        if (data.baseColor() != color) {
            data.setBaseColor(color);
            emit q->baseColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setBaseColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setFocusColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitFocusColor || method != Inherit) {
        explicitFocusColor = method == Explicit;
        if (data.focusColor() != color) {
            data.setFocusColor(color);
            emit q->focusColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setFocusColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setFrameColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitFrameColor || method != Inherit) {
        explicitFrameColor = method == Explicit;
        if (data.frameColor() != color) {
            data.setFrameColor(color);
            emit q->frameColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setFrameColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setPressColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitPressColor || method != Inherit) {
        explicitPressColor = method == Explicit;
        if (data.pressColor() != color) {
            data.setPressColor(color);
            emit q->pressColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setPressColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setSelectedTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitSelectedTextColor || method != Inherit) {
        explicitSelectedTextColor = method == Explicit;
        if (data.selectedTextColor() != color) {
            data.setSelectedTextColor(color);
            q->selectedTextColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setSelectedTextColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setSelectionColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitSelectionColor || method != Inherit) {
        explicitSelectionColor = method == Explicit;
        if (data.selectionColor() != color) {
            data.setSelectionColor(color);
            emit q->selectionColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setSelectionColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setShadowColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitShadowColor || method != Inherit) {
        explicitShadowColor = method == Explicit;
        if (data.shadowColor() != color) {
            data.setShadowColor(color);
            emit q->shadowColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setShadowColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setTextColor(const QColor &color, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitTextColor || method != Inherit) {
        explicitTextColor = method == Explicit;
        if (data.textColor() != color) {
            data.setTextColor(color);
            emit q->textColorChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setTextColor(color, Inherit);
        }
    }
}

void QQuickStylePrivate::setPadding(int padding, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitPadding || method != Inherit) {
        explicitPadding = method == Explicit;
        if (data.padding() != padding) {
            data.setPadding(padding);
            emit q->paddingChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setPadding(padding, Inherit);
        }
    }
}

void QQuickStylePrivate::setRoundness(int roundness, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitRoundness || method != Inherit) {
        explicitRoundness = method == Explicit;
        if (data.roundness() != roundness) {
            data.setRoundness(roundness);
            emit q->roundnessChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setRoundness(roundness, Inherit);
        }
    }
}

void QQuickStylePrivate::setSpacing(int spacing, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitSpacing || method != Inherit) {
        explicitSpacing = method == Explicit;
        if (data.spacing() != spacing) {
            data.setSpacing(spacing);
            emit q->spacingChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setSpacing(spacing, Inherit);
        }
    }
}

void QQuickStylePrivate::setDisabledOpacity(qreal opacity, Method method)
{
    Q_Q(QQuickStyle);
    if (!explicitDisabledOpacity || method != Inherit) {
        explicitDisabledOpacity = method == Explicit;
        if (data.disabledOpacity() != opacity) {
            data.setDisabledOpacity(opacity);
            emit q->disabledOpacityChanged();

            foreach (QQuickStyle *child, childStyles)
                child->d_func()->setDisabledOpacity(opacity, Inherit);
        }
    }
}

void QQuickStylePrivate::inherit(QQuickStyle *style)
{
    setAccentColor(style->accentColor(), Inherit);
    setBackgroundColor(style->backgroundColor(), Inherit);
    setBaseColor(style->baseColor(), QQuickStylePrivate::Inherit);
    setFocusColor(style->focusColor(), Inherit);
    setFrameColor(style->frameColor(), Inherit);
    setPressColor(style->pressColor(), Inherit);
    setSelectedTextColor(style->selectedTextColor(), Inherit);
    setSelectionColor(style->selectionColor(), Inherit);
    setShadowColor(style->shadowColor(), Inherit);
    setTextColor(style->textColor(), Inherit);
    setPadding(style->padding(), Inherit);
    setRoundness(style->roundness(), Inherit);
    setSpacing(style->spacing(), Inherit);
    setDisabledOpacity(style->disabledOpacity(), Inherit);
}

const QQuickStyleData &QQuickStylePrivate::resolve() const
{
    Q_Q(const QQuickStyle);
    QQuickStyle *style = findParentStyle(const_cast<QQuickStyle *>(q));
    return style ? style->d_func()->data : *globalStyleData();
}

void QQuickStylePrivate::itemParentChanged(QQuickItem *item, QQuickItem *)
{
    QQuickStyle *style = attachedStyle(item);
    if (style) {
        QQuickStyle *parent = findParentStyle(style);
        if (parent)
            style->setParentStyle(parent);
    }
}

QQuickStyle::QQuickStyle(const QQuickStyleData &data, QObject *parent) :
    QObject(*(new QQuickStylePrivate(data)), parent)
{
    Q_D(QQuickStyle);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Parent);
}

QQuickStyle::~QQuickStyle()
{
    Q_D(QQuickStyle);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Parent);

    setParentStyle(Q_NULLPTR);
}

QQuickStyle *QQuickStyle::qmlAttachedProperties(QObject *object)
{
    QQuickStyle *style = Q_NULLPTR;
    QQuickStyle *parent = findParentStyle(object);
    if (parent) {
        style = new QQuickStyle(parent->d_func()->data, object);
        style->setParentStyle(parent);
    } else {
        style = new QQuickStyle(*globalStyleData(), object);
    }

    QList<QQuickStyle *> childStyles = findChildStyles(object);
    foreach (QQuickStyle *child, childStyles)
        child->setParentStyle(style);
    return style;
}

QQuickStyle *QQuickStyle::parentStyle() const
{
    Q_D(const QQuickStyle);
    return d->parentStyle;
}

void QQuickStyle::setParentStyle(QQuickStyle *style)
{
    Q_D(QQuickStyle);
    if (d->parentStyle != style) {
        if (d->parentStyle)
            d->parentStyle->d_func()->childStyles.remove(this);
        d->parentStyle = style;
        if (style) {
            style->d_func()->childStyles.insert(this);
            d->inherit(style);
        }
    }
}

QColor QQuickStyle::accentColor() const
{
    Q_D(const QQuickStyle);
    return d->data.accentColor();
}

void QQuickStyle::setAccentColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setAccentColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetAccentColor()
{
    Q_D(QQuickStyle);
    d->setAccentColor(d->resolve().accentColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::backgroundColor() const
{
    Q_D(const QQuickStyle);
    return d->data.backgroundColor();
}

void QQuickStyle::setBackgroundColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setBackgroundColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetBackgroundColor()
{
    Q_D(QQuickStyle);
    d->setBackgroundColor(d->resolve().backgroundColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::baseColor() const
{
    Q_D(const QQuickStyle);
    return d->data.baseColor();
}

void QQuickStyle::setBaseColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setBaseColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetBaseColor()
{
    Q_D(QQuickStyle);
    d->setBaseColor(d->resolve().baseColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::focusColor() const
{
    Q_D(const QQuickStyle);
    return d->data.focusColor();
}

void QQuickStyle::setFocusColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setFocusColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetFocusColor()
{
    Q_D(QQuickStyle);
    d->setFocusColor(d->resolve().focusColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::frameColor() const
{
    Q_D(const QQuickStyle);
    return d->data.frameColor();
}

void QQuickStyle::setFrameColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setFrameColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetFrameColor()
{
    Q_D(QQuickStyle);
    d->setFrameColor(d->resolve().frameColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::pressColor() const
{
    Q_D(const QQuickStyle);
    return d->data.pressColor();
}

void QQuickStyle::setPressColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setPressColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetPressColor()
{
    Q_D(QQuickStyle);
    d->setPressColor(d->resolve().pressColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::selectedTextColor() const
{
    Q_D(const QQuickStyle);
    return d->data.selectedTextColor();
}

void QQuickStyle::setSelectedTextColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setSelectedTextColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetSelectedTextColor()
{
    Q_D(QQuickStyle);
    d->setSelectedTextColor(d->resolve().selectedTextColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::selectionColor() const
{
    Q_D(const QQuickStyle);
    return d->data.selectionColor();
}

void QQuickStyle::setSelectionColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setSelectionColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetSelectionColor()
{
    Q_D(QQuickStyle);
    d->setSelectionColor(d->resolve().selectionColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::shadowColor() const
{
    Q_D(const QQuickStyle);
    return d->data.shadowColor();
}

void QQuickStyle::setShadowColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setShadowColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetShadowColor()
{
    Q_D(QQuickStyle);
    d->setShadowColor(d->resolve().shadowColor(), QQuickStylePrivate::Implicit);
}

QColor QQuickStyle::textColor() const
{
    Q_D(const QQuickStyle);
    return d->data.textColor();
}

void QQuickStyle::setTextColor(const QColor &color)
{
    Q_D(QQuickStyle);
    d->setTextColor(color, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetTextColor()
{
    Q_D(QQuickStyle);
    d->setTextColor(d->resolve().textColor(), QQuickStylePrivate::Implicit);
}

int QQuickStyle::padding() const
{
    Q_D(const QQuickStyle);
    return d->data.padding();
}

void QQuickStyle::setPadding(int padding)
{
    Q_D(QQuickStyle);
    d->setPadding(padding, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetPadding()
{
    Q_D(QQuickStyle);
    d->setPadding(d->resolve().padding(), QQuickStylePrivate::Implicit);
}

int QQuickStyle::roundness() const
{
    Q_D(const QQuickStyle);
    return d->data.roundness();
}

void QQuickStyle::setRoundness(int roundness)
{
    Q_D(QQuickStyle);
    d->setRoundness(roundness, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetRoundness()
{
    Q_D(QQuickStyle);
    d->setRoundness(d->resolve().roundness(), QQuickStylePrivate::Implicit);
}

int QQuickStyle::spacing() const
{
    Q_D(const QQuickStyle);
    return d->data.spacing();
}

void QQuickStyle::setSpacing(int spacing)
{
    Q_D(QQuickStyle);
    d->setSpacing(spacing, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetSpacing()
{
    Q_D(QQuickStyle);
    d->setSpacing(d->resolve().spacing(), QQuickStylePrivate::Implicit);
}

qreal QQuickStyle::disabledOpacity() const
{
    Q_D(const QQuickStyle);
    return d->data.disabledOpacity();
}

void QQuickStyle::setDisabledOpacity(qreal opacity)
{
    Q_D(QQuickStyle);
    d->setDisabledOpacity(opacity, QQuickStylePrivate::Explicit);
}

void QQuickStyle::resetDisabledOpacity()
{
    Q_D(QQuickStyle);
    d->setDisabledOpacity(d->resolve().disabledOpacity(), QQuickStylePrivate::Implicit);
}

QT_END_NAMESPACE
