// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcolorinputs_p.h"

#include <QtCore/QRegularExpression>

QT_BEGIN_NAMESPACE

QQuickColorInputs::QQuickColorInputs() = default;

QColor QQuickColorInputs::color() const
{
    return QColor::fromHsvF(m_hsva.h, m_hsva.s, m_hsva.v, m_hsva.a);
}

void QQuickColorInputs::setColor(const QColor &c)
{
    if (color().rgba() == c.rgba())
        return;

    // If we get a QColor from an Hsv or Hsl color system,
    // we want to get the raw values without the risk of QColor converting them,
    // and possible deleting relevant information for achromatic cases.
    if (c.spec() == QColor::Spec::Hsl) {
        const auto sv = getSaturationAndValue(c.hslSaturationF(), c.lightnessF());
        m_hsva.h = qBound(.0, c.hslHueF(), 1.0);
        m_hsva.s = qBound(.0, sv.first, 1.0);
        m_hsva.v = qBound(.0, sv.second, 1.0);
    } else {
        m_hsva.h = qBound(.0, c.hsvHueF(), 1.0);
        m_hsva.s = qBound(.0, c.hsvSaturationF(), 1.0);
        m_hsva.v = qBound(.0, c.valueF(), 1.0);
    }

    m_hsva.a = c.alphaF();

    emit colorChanged(color());
}

int QQuickColorInputs::red() const
{
    return color().red();
}

int QQuickColorInputs::green() const
{
    return color().green();
}

int QQuickColorInputs::blue() const
{
    return color().blue();
}

qreal QQuickColorInputs::alpha() const
{
    return m_hsva.a;
}

qreal QQuickColorInputs::hue() const
{
    return m_hsva.h;
}

qreal QQuickColorInputs::hslSaturation() const
{
    return getSaturationAndLightness(m_hsva.s, m_hsva.v).first;
}

qreal QQuickColorInputs::hsvSaturation() const
{
    return m_hsva.s;
}

qreal QQuickColorInputs::value() const
{
    return m_hsva.v;
}

qreal QQuickColorInputs::lightness() const
{
    return getSaturationAndLightness(m_hsva.s, m_hsva.v).second;
}

bool QQuickColorInputs::showAlpha() const
{
    return m_showAlpha;
}

void QQuickColorInputs::setShowAlpha(bool showAlpha)
{
    if (m_showAlpha == showAlpha)
        return;

    m_showAlpha = showAlpha;
    emit showAlphaChanged(m_showAlpha);
}

QQuickTextInput *QQuickColorInputs::hexInput() const
{
    return m_hexInput;
}

void QQuickColorInputs::setHexInput(QQuickTextInput *hexInput)
{
    if (m_hexInput == hexInput)
        return;

    if (m_hexInput)
        disconnect(m_hexInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHexChanged);

    m_hexInput = hexInput;

    if (m_hexInput)
        connect(m_hexInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHexChanged);

    emit hexInputChanged();
}

QQuickTextInput *QQuickColorInputs::redInput() const
{
    return m_redInput;
}

void QQuickColorInputs::setRedInput(QQuickTextInput *redInput)
{
    if (m_redInput == redInput)
        return;

    if (m_redInput)
        disconnect(m_redInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleRedChanged);

    m_redInput = redInput;

    if (m_redInput)
        connect(m_redInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleRedChanged);

    emit redInputChanged();
}

QQuickTextInput *QQuickColorInputs::greenInput() const
{
    return m_greenInput;
}

void QQuickColorInputs::setGreenInput(QQuickTextInput *greenInput)
{
    if (m_greenInput == greenInput)
        return;

    if (m_greenInput)
        disconnect(m_greenInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleGreenChanged);

    m_greenInput = greenInput;

    if (m_greenInput)
        connect(m_greenInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleGreenChanged);

    emit greenInputChanged();
}

QQuickTextInput *QQuickColorInputs::blueInput() const
{
    return m_blueInput;
}

void QQuickColorInputs::setBlueInput(QQuickTextInput *blueInput)
{
    if (m_blueInput == blueInput)
        return;

    if (m_blueInput)
        disconnect(m_blueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleBlueChanged);

    m_blueInput = blueInput;

    if (m_blueInput)
        connect(m_blueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleBlueChanged);

    emit blueInputChanged();
}

QQuickTextInput *QQuickColorInputs::hsvHueInput() const
{
    return m_hsvHueInput;
}

void QQuickColorInputs::setHsvHueInput(QQuickTextInput *hsvHueInput)
{
    if (m_hsvHueInput == hsvHueInput)
        return;

    if (m_hsvHueInput)
        disconnect(m_hsvHueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvHueChanged);

    m_hsvHueInput = hsvHueInput;

    if (m_hsvHueInput)
        connect(m_hsvHueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvHueChanged);

    emit hsvHueInputChanged();
}

QQuickTextInput *QQuickColorInputs::hslHueInput() const
{
    return m_hslHueInput;
}

void QQuickColorInputs::setHslHueInput(QQuickTextInput *hslHueInput)
{
    if (m_hslHueInput == hslHueInput)
        return;

    if (m_hslHueInput)
        disconnect(m_hslHueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslHueChanged);

    m_hslHueInput = hslHueInput;

    if (m_hslHueInput)
        connect(m_hslHueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslHueChanged);

    emit hslHueInputChanged();
}

QQuickTextInput *QQuickColorInputs::hsvSaturationInput() const
{
    return m_hsvSaturationInput;
}

void QQuickColorInputs::setHsvSaturationInput(QQuickTextInput *hsvSaturationInput)
{
    if (m_hsvSaturationInput == hsvSaturationInput)
        return;

    if (m_hsvSaturationInput)
        disconnect(m_hsvSaturationInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvSaturationChanged);

    m_hsvSaturationInput = hsvSaturationInput;

    if (m_hsvSaturationInput)
        connect(m_hsvSaturationInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvSaturationChanged);

    emit hsvSaturationInputChanged();
}

QQuickTextInput *QQuickColorInputs::hslSaturationInput() const
{
    return m_hslSaturationInput;
}

void QQuickColorInputs::setHslSaturationInput(QQuickTextInput *hslSaturationInput)
{
    if (m_hslSaturationInput == hslSaturationInput)
        return;

    if (m_hslSaturationInput)
        disconnect(m_hslSaturationInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslSaturationChanged);

    m_hslSaturationInput = hslSaturationInput;

    if (m_hslSaturationInput)
        connect(m_hslSaturationInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslSaturationChanged);

    emit hslSaturationInputChanged();
}

QQuickTextInput *QQuickColorInputs::valueInput() const
{
    return m_valueInput;
}

void QQuickColorInputs::setValueInput(QQuickTextInput *valueInput)
{
    if (m_valueInput == valueInput)
        return;

    if (m_valueInput)
        disconnect(m_valueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleValueChanged);

    m_valueInput = valueInput;

    if (m_valueInput)
        connect(m_valueInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleValueChanged);

    emit valueInputChanged();
}

QQuickTextInput *QQuickColorInputs::lightnessInput() const
{
    return m_lightnessInput;
}

void QQuickColorInputs::setLightnessInput(QQuickTextInput *lightnessInput)
{
    if (m_lightnessInput == lightnessInput)
        return;

    if (m_lightnessInput)
        disconnect(m_lightnessInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleLightnessChanged);

    m_lightnessInput = lightnessInput;

    if (m_lightnessInput)
        connect(m_lightnessInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleLightnessChanged);

    emit lightnessInputChanged();
}

QQuickTextInput *QQuickColorInputs::rgbAlphaInput() const
{
    return m_rgbAlphaInput;
}

void QQuickColorInputs::setRgbAlphaInput(QQuickTextInput *alphaInput)
{
    if (alphaInput == m_rgbAlphaInput)
        return;

    if (m_rgbAlphaInput) {
        disconnect(m_rgbAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleRgbAlphaChanged);
        disconnect(this, &QQuickColorInputs::showAlphaChanged, m_rgbAlphaInput, &QQuickTextInput::setVisible);
    }

    m_rgbAlphaInput = alphaInput;

    if (m_rgbAlphaInput) {
        connect(m_rgbAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleRgbAlphaChanged);
        connect(this, &QQuickColorInputs::showAlphaChanged, m_rgbAlphaInput, &QQuickTextInput::setVisible);
        m_rgbAlphaInput->setVisible(showAlpha());
    }

    emit rgbAlphaInputChanged();
}

QQuickTextInput *QQuickColorInputs::hsvAlphaInput() const
{
    return m_hsvAlphaInput;
}

void QQuickColorInputs::setHsvAlphaInput(QQuickTextInput *alphaInput)
{
    if (alphaInput == m_hsvAlphaInput)
        return;

    if (m_hsvAlphaInput) {
        disconnect(m_hsvAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvAlphaChanged);
        disconnect(this, &QQuickColorInputs::showAlphaChanged, m_hsvAlphaInput, &QQuickTextInput::setVisible);
    }

    m_hsvAlphaInput = alphaInput;

    if (m_hsvAlphaInput) {
        connect(m_hsvAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHsvAlphaChanged);
        connect(this, &QQuickColorInputs::showAlphaChanged, m_hsvAlphaInput, &QQuickTextInput::setVisible);
        m_hsvAlphaInput->setVisible(showAlpha());
    }

    emit hsvAlphaInputChanged();
}

QQuickTextInput *QQuickColorInputs::hslAlphaInput() const
{
    return m_hslAlphaInput;
}

void QQuickColorInputs::setHslAlphaInput(QQuickTextInput *alphaInput)
{
    if (alphaInput == m_hslAlphaInput)
        return;

    if (m_hslAlphaInput) {
        disconnect(m_hslAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslAlphaChanged);
        disconnect(this, &QQuickColorInputs::showAlphaChanged, m_hslAlphaInput, &QQuickTextInput::setVisible);
    }

    m_hslAlphaInput = alphaInput;

    if (m_hslAlphaInput) {
        connect(m_hslAlphaInput, &QQuickTextInput::editingFinished, this, &QQuickColorInputs::handleHslAlphaChanged);
        connect(this, &QQuickColorInputs::showAlphaChanged, m_hslAlphaInput, &QQuickTextInput::setVisible);
        m_hslAlphaInput->setVisible(showAlpha());
    }

    emit hslAlphaInputChanged();
}

void QQuickColorInputs::handleHexChanged()
{
    emit colorModified(QColor::fromString(m_hexInput->text()));
}

void QQuickColorInputs::handleRedChanged()
{
    QColor c = color();
    c.setRed(qBound(0, m_redInput->text().toInt(), 255));
    emit colorModified(c);
}

void QQuickColorInputs::handleGreenChanged()
{
    QColor c = color();
    c.setGreen(qBound(0, m_greenInput->text().toInt(), 255));
    emit colorModified(c);
}

void QQuickColorInputs::handleBlueChanged()
{
    QColor c = color();
    c.setBlue(qBound(0, m_blueInput->text().toInt(), 255));
    emit colorModified(c);
}

static QString s_percentage_pattern = QString::fromUtf8("^(\\d+)%?$");
static QString s_degree_pattern = QString::fromUtf8("(\\d+)°?$");

void QQuickColorInputs::handleHsvHueChanged()
{
    const QRegularExpression pattern(s_degree_pattern);
    const auto match = pattern.match(m_hsvHueInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 360)) / static_cast<qreal>(360);
        emit colorModified(QColor::fromHsvF(input, hsvSaturation(), value(), alpha()));
    }
}

void QQuickColorInputs::handleHslHueChanged()
{
    const QRegularExpression pattern(s_degree_pattern);
    const auto match = pattern.match(m_hslHueInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 360)) / static_cast<qreal>(360);
        emit colorModified(QColor::fromHslF(input, hslSaturation(), lightness(), alpha()));
    }
}

void QQuickColorInputs::handleHsvSaturationChanged()
{
    const QRegularExpression pattern(s_percentage_pattern);
    const auto match = pattern.match(m_hsvSaturationInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 100)) / static_cast<qreal>(100);
        emit colorModified(QColor::fromHsvF(hue(), input, value(), alpha()));
    }
}

void QQuickColorInputs::handleHslSaturationChanged()
{
    const QRegularExpression pattern(s_percentage_pattern);
    const auto match = pattern.match(m_hslSaturationInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 100)) / static_cast<qreal>(100);
        emit colorModified(QColor::fromHslF(hue(), input, lightness(), alpha()));
    }
}

void QQuickColorInputs::handleValueChanged()
{
    const QRegularExpression pattern(s_percentage_pattern);
    const auto match = pattern.match(m_valueInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 100)) / static_cast<qreal>(100);
        emit colorModified(QColor::fromHsvF(hue(), hsvSaturation(), input, alpha()));
    }
}

void QQuickColorInputs::handleLightnessChanged()
{
    const QRegularExpression pattern(s_percentage_pattern);
    const auto match = pattern.match(m_lightnessInput->text());
    if (match.hasMatch()) {
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 100)) / static_cast<qreal>(100);
        emit colorModified(QColor::fromHslF(hue(), hslSaturation(), input, alpha()));
    }
}

void QQuickColorInputs::handleRgbAlphaChanged()
{
    handleAlphaChanged(m_rgbAlphaInput->text());
}

void QQuickColorInputs::handleHsvAlphaChanged()
{
    handleAlphaChanged(m_hsvAlphaInput->text());
}

void QQuickColorInputs::handleHslAlphaChanged()
{
    handleAlphaChanged(m_hslAlphaInput->text());
}

void QQuickColorInputs::handleAlphaChanged(const QString &input)
{
    const QRegularExpression pattern(s_percentage_pattern);
    const auto match = pattern.match(input);
    if (match.hasMatch()) {
        QColor c = color();
        const auto substr = match.captured(1);
        const qreal input = static_cast<qreal>(qBound(0, substr.toInt(), 100)) / static_cast<qreal>(100);
        c.setAlphaF(input);
        emit colorModified(c);
    }
}

QT_END_NAMESPACE
