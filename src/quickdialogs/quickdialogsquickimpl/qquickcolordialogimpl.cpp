// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcolordialogimpl_p.h"
#include "qquickcolordialogimpl_p_p.h"

#include "qquickcolordialogutils_p.h"

#include <QtQuickTemplates2/private/qquickslider_p.h>

#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QColor grabScreenColor(const QPoint &p)
{
    QScreen *screen = QGuiApplication::screenAt(p);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect screenRect = screen->geometry();
    const QPixmap pixmap =
            screen->grabWindow(0, p.x() - screenRect.x(), p.y() - screenRect.y(), 1, 1);
    const QImage i = pixmap.toImage();
    return i.pixel(0, 0);
}

bool QQuickEyeDropperEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseMove: {
        m_lastPosition = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
        m_update(m_lastPosition);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        m_lastPosition = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
        m_leave(m_lastPosition, QQuickEyeDropperEventFilter::LeaveReason::Default);
        return true;
    }
    case QEvent::MouseButtonPress:
        return true;
    case QEvent::KeyPress: {
        auto keyEvent = static_cast<QKeyEvent *>(event);
#if QT_CONFIG(shortcut)
        if (keyEvent->matches(QKeySequence::Cancel))
            m_leave(m_lastPosition, QQuickEyeDropperEventFilter::LeaveReason::Cancel);
        else
#endif
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            m_leave(m_lastPosition, QQuickEyeDropperEventFilter::LeaveReason::Default);
        } else if (keyEvent->key() == Qt::Key_Escape) {
            m_leave(m_lastPosition, QQuickEyeDropperEventFilter::LeaveReason::Cancel);
        }
        keyEvent->accept();
        return true;
    }
    default:
        return QObject::eventFilter(obj, event);
    }
}

QQuickColorDialogImplPrivate::QQuickColorDialogImplPrivate() = default;

QQuickColorDialogImplPrivate::~QQuickColorDialogImplPrivate()
{
    if (m_eyeDropperMode)
        eyeDropperLeave(QCursor::pos(), QQuickEyeDropperEventFilter::LeaveReason::Default);
}

void QQuickColorDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickColorDialogImpl);
    const auto c = q->color();
    if (buttonRole(button) == QPlatformDialogHelper::AcceptRole && c.isValid()) {
        q->setColor(c);
        q->accept();
    }
    QQuickDialogPrivate::handleClick(button);
}

QQuickColorDialogImplAttached *QQuickColorDialogImplPrivate::attachedOrWarn()
{
    Q_Q(QQuickColorDialogImpl);
    QQuickColorDialogImplAttached *attached = static_cast<QQuickColorDialogImplAttached *>(
            qmlAttachedPropertiesObject<QQuickColorDialogImpl>(q, false));
    if (!attached)
        qmlWarning(q) << "Expected ColorDialogImpl attached object to be present on" << this;
    return attached;
}

void QQuickColorDialogImplPrivate::eyeDropperEnter()
{
    Q_Q(const QQuickColorDialogImpl);
    if (m_eyeDropperMode)
        return;

    if (m_eyeDropperWindow.isNull()) {
        if (window.isNull()) {
            qWarning() << "No window found, cannot enter eyeDropperMode.";
            return;
        }

        m_eyeDropperWindow = window;
    }

    m_eyeDropperPreviousColor = q->color();

    if (!bool(eyeDropperEventFilter))
        eyeDropperEventFilter.reset(new QQuickEyeDropperEventFilter(
                [this](QPoint pos, QQuickEyeDropperEventFilter::LeaveReason c) {
                    eyeDropperLeave(pos, c);
                },
                [this](QPoint pos) { eyeDropperPointerMoved(pos); }));

    if (m_eyeDropperWindow->setMouseGrabEnabled(true)) {
#if QT_CONFIG(cursor)
        QGuiApplication::setOverrideCursor(Qt::CrossCursor);
#endif
        m_eyeDropperWindow->installEventFilter(eyeDropperEventFilter.get());
        m_eyeDropperMode = true;
    }
}

void QQuickColorDialogImplPrivate::eyeDropperLeave(
        const QPoint &pos, QQuickEyeDropperEventFilter::LeaveReason actionOnLeave)
{
    Q_Q(QQuickColorDialogImpl);

    if (!m_eyeDropperMode)
        return;

    if (!m_eyeDropperWindow) {
        qWarning() << "Window not set, cannot leave eyeDropperMode.";
        return;
    }

    const QColor colorToUse = actionOnLeave == QQuickEyeDropperEventFilter::LeaveReason::Cancel
            ? m_eyeDropperPreviousColor
            : grabScreenColor(pos);
    q->setColor(colorToUse);

    m_eyeDropperWindow->removeEventFilter(eyeDropperEventFilter.get());
    m_eyeDropperWindow->setMouseGrabEnabled(false);
#if QT_CONFIG(cursor)
    QGuiApplication::restoreOverrideCursor();
#endif

    m_eyeDropperMode = false;
    m_eyeDropperWindow.clear();
}

void QQuickColorDialogImplPrivate::eyeDropperPointerMoved(const QPoint &pos)
{
    Q_Q(QQuickColorDialogImpl);
    q->setColor(grabScreenColor(pos));
}

void QQuickColorDialogImplPrivate::alphaSliderMoved()
{
    Q_Q(QQuickColorDialogImpl);
    if (auto attached = attachedOrWarn())
        q->setAlpha(attached->alphaSlider()->value());
}

QQuickColorDialogImpl::QQuickColorDialogImpl(QObject *parent)
    : QQuickDialog(*(new QQuickColorDialogImplPrivate), parent)
{
}

QQuickColorDialogImplAttached *QQuickColorDialogImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickColorDialogImplAttached(object);
}

QColor QQuickColorDialogImpl::color() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsl ? QColor::fromHslF(d->m_hsva.h, d->m_hsva.s, d->m_hsva.l, d->m_hsva.a)
                    : QColor::fromHsvF(d->m_hsva.h, d->m_hsva.s, d->m_hsva.v, d->m_hsva.a);
}

void QQuickColorDialogImpl::setColor(const QColor &c)
{
    Q_D(QQuickColorDialogImpl);
    if (color().rgba() == c.rgba())
        return;

    // If we get a QColor from an Hsv or Hsl color system,
    // we want to get the raw values without the risk of QColor converting them,
    // and possible deleting relevant information for achromatic cases.
    if (c.spec() == QColor::Spec::Hsv) {
        d->m_hsva.h = qBound(.0, c.hsvHueF(), 1.0);
        if (d->m_hsl) {
            const auto sl = getSaturationAndLightness(c.hsvSaturationF(), c.valueF());
            d->m_hsva.s = qBound(.0, sl.first, 1.0);
            d->m_hsva.l = qBound(.0, sl.second, 1.0);
        } else {
            d->m_hsva.s = qBound(.0, c.hsvSaturationF(), 1.0);
            d->m_hsva.v = qBound(.0, c.valueF(), 1.0);
        }
    } else if (c.spec() == QColor::Spec::Hsl) {
        d->m_hsva.h = qBound(.0, c.hslHueF(), 1.0);
        if (d->m_hsl) {
            d->m_hsva.s = qBound(.0, c.hslSaturationF(), 1.0);
            d->m_hsva.l = qBound(.0, c.lightnessF(), 1.0);
        } else {
            const auto sv = getSaturationAndValue(c.hslSaturationF(), c.lightnessF());
            d->m_hsva.s = qBound(.0, sv.first, 1.0);
            d->m_hsva.v = qBound(.0, sv.second, 1.0);
        }
    } else {
        d->m_hsva.h = qBound(.0, d->m_hsl ? c.hslHueF() : c.hsvHueF(), 1.0);
        d->m_hsva.s = qBound(.0, d->m_hsl ? c.hslSaturationF() : c.hsvSaturationF(), 1.0);
        d->m_hsva.v = qBound(.0, d->m_hsl ? c.lightnessF() : c.valueF(), 1.0);
    }

    d->m_hsva.a = c.alphaF();

    emit colorChanged(color());
}

int QQuickColorDialogImpl::red() const
{
    return color().red();
}

void QQuickColorDialogImpl::setRed(int red)
{
    Q_D(QQuickColorDialogImpl);

    auto c = color();

    if (c.red() == red)
        return;

    c.setRed(red);

    d->m_hsva.h = d->m_hsl ? c.hslHueF() : c.hsvHueF();
    d->m_hsva.s = d->m_hsl ? c.hslSaturationF() : c.hsvSaturationF();
    d->m_hsva.v = d->m_hsl ? c.lightnessF() : c.valueF();
    d->m_hsva.a = c.alphaF();

    emit colorChanged(c);
}

int QQuickColorDialogImpl::green() const
{
    return color().green();
}

void QQuickColorDialogImpl::setGreen(int green)
{
    Q_D(QQuickColorDialogImpl);

    auto c = color();

    if (c.green() == green)
        return;

    c.setGreen(green);

    d->m_hsva.h = d->m_hsl ? c.hslHueF() : c.hsvHueF();
    d->m_hsva.s = d->m_hsl ? c.hslSaturationF() : c.hsvSaturationF();
    d->m_hsva.v = d->m_hsl ? c.lightnessF() : c.valueF();
    d->m_hsva.a = c.alphaF();

    emit colorChanged(c);
}

int QQuickColorDialogImpl::blue() const
{
    return color().blue();
}

void QQuickColorDialogImpl::setBlue(int blue)
{
    Q_D(QQuickColorDialogImpl);

    auto c = color();

    if (c.blue() == blue)
        return;

    c.setBlue(blue);

    d->m_hsva.h = d->m_hsl ? c.hslHueF() : c.hsvHueF();
    d->m_hsva.s = d->m_hsl ? c.hslSaturationF() : c.hsvSaturationF();
    d->m_hsva.v = d->m_hsl ? c.lightnessF() : c.valueF();
    d->m_hsva.a = c.alphaF();

    emit colorChanged(c);
}

qreal QQuickColorDialogImpl::alpha() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsva.a;
}

void QQuickColorDialogImpl::setAlpha(qreal alpha)
{
    Q_D(QQuickColorDialogImpl);

    if (!qt_is_finite(alpha))
        return;

    alpha = qBound(.0, alpha, 1.0);

    if (qFuzzyCompare(d->m_hsva.a, alpha))
        return;

    d->m_hsva.a = alpha;

    emit colorChanged(color());
}

qreal QQuickColorDialogImpl::hue() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsva.h;
}

void QQuickColorDialogImpl::setHue(qreal hue)
{
    Q_D(QQuickColorDialogImpl);

    if (!qt_is_finite(hue))
        return;

    d->m_hsva.h = hue;

    emit colorChanged(color());
}

qreal QQuickColorDialogImpl::saturation() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsva.s;
}

void QQuickColorDialogImpl::setSaturation(qreal saturation)
{
    Q_D(QQuickColorDialogImpl);
    if (!qt_is_finite(saturation))
        return;

    d->m_hsva.s = saturation;

    emit colorChanged(color());
}

qreal QQuickColorDialogImpl::value() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsl ? getSaturationAndValue(d->m_hsva.s, d->m_hsva.l).second : d->m_hsva.v;
}

void QQuickColorDialogImpl::setValue(qreal value)
{
    Q_D(QQuickColorDialogImpl);
    if (!qt_is_finite(value))
        return;

    d->m_hsva.v = value;

    if (d->m_hsl)
        d->m_hsva.s = getSaturationAndValue(d->m_hsva.s, d->m_hsva.l).first;

    d->m_hsl = false;
    emit colorChanged(color());
}

qreal QQuickColorDialogImpl::lightness() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsl ? d->m_hsva.l : getSaturationAndLightness(d->m_hsva.s, d->m_hsva.v).second;
}

void QQuickColorDialogImpl::setLightness(qreal lightness)
{
    Q_D(QQuickColorDialogImpl);
    if (!qt_is_finite(lightness))
        return;

    d->m_hsva.l = lightness;

    if (!d->m_hsl)
        d->m_hsva.s = getSaturationAndLightness(d->m_hsva.s, d->m_hsva.v).first;

    d->m_hsl = true;
    emit colorChanged(color());
}

bool QQuickColorDialogImpl::isHsl() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->m_hsl;
}

void QQuickColorDialogImpl::setHsl(bool hsl)
{
    Q_D(QQuickColorDialogImpl);

    if (d->m_hsl == hsl)
        return;

    d->m_hsl = hsl;
    emit specChanged();
}

QSharedPointer<QColorDialogOptions> QQuickColorDialogImpl::options() const
{
    Q_D(const QQuickColorDialogImpl);
    return d->options;
}

void QQuickColorDialogImpl::setOptions(const QSharedPointer<QColorDialogOptions> &options)
{
    Q_D(QQuickColorDialogImpl);
    d->options = options;

    QQuickColorDialogImplAttached *attached = d->attachedOrWarn();

    if (attached) {
        const bool screenGrabbingAllowed = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ScreenWindowGrabbing);
        const bool offscreen = qgetenv("QT_QPA_PLATFORM").compare(QLatin1String("offscreen"), Qt::CaseInsensitive) == 0;
        attached->eyeDropperButton()->setVisible(screenGrabbingAllowed && !offscreen);

        if (d->options) {
            attached->buttonBox()->setVisible(
                    !(d->options->options() & QColorDialogOptions::NoButtons));

            const bool showAlpha = d->options->options() & QColorDialogOptions::ShowAlphaChannel;
            attached->alphaSlider()->setVisible(showAlpha);
            attached->colorInputs()->setShowAlpha(showAlpha);
        }
    }
}

void QQuickColorDialogImpl::invokeEyeDropper()
{
    Q_D(QQuickColorDialogImpl);
    d->eyeDropperEnter();
}

QQuickColorDialogImplAttached::QQuickColorDialogImplAttached(QObject *parent)
    : QObject(*(new QQuickColorDialogImplAttachedPrivate), parent)
{
    if (!qobject_cast<QQuickColorDialogImpl *>(parent)) {
        qmlWarning(this) << "ColorDialogImpl attached properties should only be "
                         << "accessed through the root ColorDialogImpl instance";
    }
}

QQuickDialogButtonBox *QQuickColorDialogImplAttached::buttonBox() const
{
    Q_D(const QQuickColorDialogImplAttached);
    return d->buttonBox;
}

void QQuickColorDialogImplAttached::setButtonBox(QQuickDialogButtonBox *buttonBox)
{
    Q_D(QQuickColorDialogImplAttached);
    if (d->buttonBox == buttonBox)
        return;

    if (d->buttonBox) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(colorDialogImpl);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::clicked,
                dialogPrivate, &QQuickDialogPrivate::handleClick);
        }
    }

    d->buttonBox = buttonBox;

    if (d->buttonBox) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(colorDialogImpl);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::clicked,
                dialogPrivate, &QQuickDialogPrivate::handleClick);
        }
    }

    emit buttonBoxChanged();
}

QQuickAbstractButton *QQuickColorDialogImplAttached::eyeDropperButton() const
{
    Q_D(const QQuickColorDialogImplAttached);
    return d->eyeDropperButton;
}

void QQuickColorDialogImplAttached::setEyeDropperButton(QQuickAbstractButton *eyeDropperButton)
{
    Q_D(QQuickColorDialogImplAttached);
    Q_ASSERT(!d->eyeDropperButton);
    if (d->eyeDropperButton == eyeDropperButton)
        return;

    d->eyeDropperButton = eyeDropperButton;
    if (auto dialog = qobject_cast<QQuickColorDialogImpl *>(parent()))
        connect(d->eyeDropperButton, &QQuickAbstractButton::clicked, dialog, &QQuickColorDialogImpl::invokeEyeDropper);
    emit eyeDropperButtonChanged();
}

QQuickAbstractColorPicker *QQuickColorDialogImplAttached::colorPicker() const
{
    Q_D(const QQuickColorDialogImplAttached);
    return d->colorPicker;
}
void QQuickColorDialogImplAttached::setColorPicker(QQuickAbstractColorPicker *colorPicker)
{
    Q_D(QQuickColorDialogImplAttached);
    if (d->colorPicker == colorPicker)
        return;

    if (d->colorPicker) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            QObject::disconnect(d->colorPicker, &QQuickAbstractColorPicker::colorPicked,
                colorDialogImpl, &QQuickColorDialogImpl::setColor);
        }
    }

    d->colorPicker = colorPicker;

    if (d->colorPicker) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            QObject::connect(d->colorPicker, &QQuickAbstractColorPicker::colorPicked,
                colorDialogImpl, &QQuickColorDialogImpl::setColor);
        }
    }

    emit colorPickerChanged();
}

QQuickSlider *QQuickColorDialogImplAttached::alphaSlider() const
{
    Q_D(const QQuickColorDialogImplAttached);
    return d->alphaSlider;
}

void QQuickColorDialogImplAttached::setAlphaSlider(QQuickSlider *alphaSlider)
{
    Q_D(QQuickColorDialogImplAttached);
    if (d->alphaSlider == alphaSlider)
        return;

    if (d->alphaSlider) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            auto dialogPrivate = QQuickColorDialogImplPrivate::get(colorDialogImpl);
            QObjectPrivate::disconnect(d->alphaSlider, &QQuickSlider::moved,
                dialogPrivate, &QQuickColorDialogImplPrivate::alphaSliderMoved);
        }
    }

    d->alphaSlider = alphaSlider;

    if (d->alphaSlider) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl) {
            auto dialogPrivate = QQuickColorDialogImplPrivate::get(colorDialogImpl);
            QObjectPrivate::connect(d->alphaSlider, &QQuickSlider::moved,
                dialogPrivate, &QQuickColorDialogImplPrivate::alphaSliderMoved);
        }
    }

    emit alphaSliderChanged();
}

QQuickColorInputs *QQuickColorDialogImplAttached::colorInputs() const
{
    Q_D(const QQuickColorDialogImplAttached);
    return d->colorInputs;
}

void QQuickColorDialogImplAttached::setColorInputs(QQuickColorInputs *colorInputs)
{
    Q_D(QQuickColorDialogImplAttached);

    if (d->colorInputs == colorInputs)
        return;

    if (d->colorInputs) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl)
            QObject::disconnect(d->colorInputs, &QQuickColorInputs::colorModified,
                                colorDialogImpl, &QQuickColorDialogImpl::setColor);
    }

    d->colorInputs = colorInputs;

    if (d->colorInputs) {
        QQuickColorDialogImpl *colorDialogImpl = qobject_cast<QQuickColorDialogImpl *>(parent());
        if (colorDialogImpl)
            QObject::connect(d->colorInputs, &QQuickColorInputs::colorModified,
                             colorDialogImpl, &QQuickColorDialogImpl::setColor);
    }

    emit colorInputsChanged();
}

QT_END_NAMESPACE
