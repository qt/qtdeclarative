// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfontdialogimpl_p.h"
#include "qquickfontdialogimpl_p_p.h"

#include <QtQuickTemplates2/private/qquickdialogbuttonbox_p_p.h>
#include <private/qfontdatabase_p.h>

#include <QRegularExpression>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcAttachedProperty, "qt.quick.dialogs.quickfontdialogimpl.attachedOrWarn")

QQuickFontDialogImplPrivate::QQuickFontDialogImplPrivate()
{
}

QQuickFontDialogImplAttached *QQuickFontDialogImplPrivate::attachedOrWarn()
{
    Q_Q(QQuickFontDialogImpl);
    QQuickFontDialogImplAttached *attached = static_cast<QQuickFontDialogImplAttached *>(
            qmlAttachedPropertiesObject<QQuickFontDialogImpl>(q));
    if (!attached) {
        qCWarning(lcAttachedProperty)
                << "Expected FontDialogImpl attached object to be present on" << this;
    }
    return attached;
}

void QQuickFontDialogImplPrivate::handleAccept() { }

void QQuickFontDialogImplPrivate::handleClick(QQuickAbstractButton *button)
{
    Q_Q(QQuickFontDialogImpl);
    if (buttonRole(button) == QPlatformDialogHelper::AcceptRole) {
        q->accept();
        QQuickDialogPrivate::handleClick(button);
    }
}

QQuickFontDialogImpl::QQuickFontDialogImpl(QObject *parent)
    : QQuickDialog(*(new QQuickFontDialogImplPrivate), parent)
{
}

QQuickFontDialogImplAttached *QQuickFontDialogImpl::qmlAttachedProperties(QObject *object)
{
    return new QQuickFontDialogImplAttached(object);
}

QSharedPointer<QFontDialogOptions> QQuickFontDialogImpl::options() const
{
    Q_D(const QQuickFontDialogImpl);

    return d->options;
}

void QQuickFontDialogImpl::setOptions(const QSharedPointer<QFontDialogOptions> &options)
{
    Q_D(QQuickFontDialogImpl);

    if (options == d->options)
        return;

    d->options = options;

    emit optionsChanged();
}

QFont QQuickFontDialogImpl::currentFont() const
{
    Q_D(const QQuickFontDialogImpl);
    return d->currentFont;
}

void QQuickFontDialogImpl::setCurrentFont(const QFont &font, bool selectInListViews)
{
    Q_D(QQuickFontDialogImpl);

    if (font == d->currentFont)
        return;

    d->currentFont = font;

    emit currentFontChanged(font);

    if (!selectInListViews)
        return;

    QQuickFontDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    if (!attached->familyListView()->model().isValid()) {
        const QSignalBlocker blocker(attached->sampleEdit());
        attached->updateFamilies();
    }

    attached->selectFontInListViews(font);
}

void QQuickFontDialogImpl::init()
{
    Q_D(QQuickFontDialogImpl);
    QQuickFontDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    if (!attached->familyListView()->model().isValid())
        attached->updateFamilies();

    attached->buttonBox()->setVisible(!(options()->options() & QFontDialogOptions::NoButtons));
}

void QQuickFontDialogImpl::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickFontDialogImpl);

    QQuickDialog::keyReleaseEvent(event);

    QQuickFontDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    // The family and style text edits are read-only so that they
    // can show the current selection but also allow key input to "search".
    // This is why we handle just the release event, and don't accept it.
    if (window()->activeFocusItem() == attached->familyEdit())
        attached->searchFamily(event->text());
    else if (window()->activeFocusItem() == attached->styleEdit())
        attached->searchStyle(event->text());
}

void QQuickFontDialogImpl::focusOutEvent(QFocusEvent *event)
{
    Q_D(QQuickFontDialogImpl);

    QQuickDialog::focusOutEvent(event);

    QQuickFontDialogImplAttached *attached = d->attachedOrWarn();
    if (!attached)
        return;

    attached->clearSearch();
}

QQuickFontDialogImplAttached::QQuickFontDialogImplAttached(QObject *parent)
    : QObject(*(new QQuickFontDialogImplAttachedPrivate), parent),
      m_writingSystem(QFontDatabase::Any),
      m_selectedSize(-1),
      m_smoothlyScalable(false),
      m_ignoreFamilyUpdate(false),
      m_ignoreStyleUpdate(false)
{
    if (!qobject_cast<QQuickFontDialogImpl *>(parent)) {
        qmlWarning(this) << "FontDialogImpl attached properties should only be "
                         << "accessed through the root FileDialogImpl instance";
    }
}

QQuickListView *QQuickFontDialogImplAttached::familyListView() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->familyListView;
}

void QQuickFontDialogImplAttached::setFamilyListView(QQuickListView *familyListView)
{
    Q_D(QQuickFontDialogImplAttached);
    if (d->familyListView == familyListView)
        return;

    if (d->familyListView) {
        disconnect(d->familyListView, &QQuickListView::currentIndexChanged,
                   this, &QQuickFontDialogImplAttached::_q_familyChanged);
    }

    d->familyListView = familyListView;

    if (familyListView) {
        connect(d->familyListView, &QQuickListView::currentIndexChanged,
                this, &QQuickFontDialogImplAttached::_q_familyChanged);
    }

    emit familyListViewChanged();
}

QQuickListView *QQuickFontDialogImplAttached::styleListView() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->styleListView;
}

void QQuickFontDialogImplAttached::setStyleListView(QQuickListView *styleListView)
{
    Q_D(QQuickFontDialogImplAttached);
    if (d->styleListView == styleListView)
        return;

    if (d->styleListView) {
        disconnect(d->styleListView, &QQuickListView::currentIndexChanged,
                   this, &QQuickFontDialogImplAttached::_q_styleChanged);
    }

    d->styleListView = styleListView;

    if (styleListView) {
        connect(d->styleListView, &QQuickListView::currentIndexChanged,
                this, &QQuickFontDialogImplAttached::_q_styleChanged);
    }

    emit styleListViewChanged();
}

QQuickListView *QQuickFontDialogImplAttached::sizeListView() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->sizeListView;
}

void QQuickFontDialogImplAttached::setSizeListView(QQuickListView *sizeListView)
{
    Q_D(QQuickFontDialogImplAttached);
    if (d->sizeListView == sizeListView)
        return;

    if (d->sizeListView) {
        disconnect(d->sizeListView, &QQuickListView::currentIndexChanged,
                   this, &QQuickFontDialogImplAttached::_q_sizeChanged);
    }

    d->sizeListView = sizeListView;

    if (d->sizeListView) {
        connect(d->sizeListView, &QQuickListView::currentIndexChanged,
                this, &QQuickFontDialogImplAttached::_q_sizeChanged);
    }

    emit sizeListViewChanged();
}

QQuickTextEdit *QQuickFontDialogImplAttached::sampleEdit() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->sampleEdit;
}

void QQuickFontDialogImplAttached::setSampleEdit(QQuickTextEdit *sampleEdit)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->sampleEdit == sampleEdit)
        return;

    if (d->sampleEdit) {
        QObjectPrivate::disconnect(d->sampleEdit, &QQuickTextEdit::fontChanged,
                                   d, &QQuickFontDialogImplAttachedPrivate::currentFontChanged);
    }

    d->sampleEdit = sampleEdit;

    if (d->sampleEdit) {
        QObjectPrivate::connect(d->sampleEdit, &QQuickTextEdit::fontChanged,
                                d, &QQuickFontDialogImplAttachedPrivate::currentFontChanged);

        d->sampleEdit->setText(QFontDatabase::writingSystemSample(m_writingSystem));
    }

    emit sampleEditChanged();
}

QQuickDialogButtonBox *QQuickFontDialogImplAttached::buttonBox() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->buttonBox;
}

void QQuickFontDialogImplAttached::setButtonBox(QQuickDialogButtonBox *buttonBox)
{
    Q_D(QQuickFontDialogImplAttached);
    if (buttonBox == d->buttonBox)
        return;

    if (d->buttonBox) {
        QQuickFontDialogImpl *fontDialogImpl = qobject_cast<QQuickFontDialogImpl *>(parent());
        if (fontDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(fontDialogImpl);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::accepted,
                                       dialogPrivate, &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::rejected,
                                       dialogPrivate, &QQuickDialogPrivate::handleReject);
            QObjectPrivate::disconnect(d->buttonBox, &QQuickDialogButtonBox::clicked, dialogPrivate,
                                       &QQuickDialogPrivate::handleClick);
        }
    }

    d->buttonBox = buttonBox;

    if (buttonBox) {
        QQuickFontDialogImpl *fontDialogImpl = qobject_cast<QQuickFontDialogImpl *>(parent());
        if (fontDialogImpl) {
            auto dialogPrivate = QQuickDialogPrivate::get(fontDialogImpl);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::accepted, dialogPrivate,
                                    &QQuickDialogPrivate::handleAccept);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::rejected, dialogPrivate,
                                    &QQuickDialogPrivate::handleReject);
            QObjectPrivate::connect(d->buttonBox, &QQuickDialogButtonBox::clicked, dialogPrivate,
                                    &QQuickDialogPrivate::handleClick);
        }
    }

    emit buttonBoxChanged();
}

QQuickComboBox *QQuickFontDialogImplAttached::writingSystemComboBox() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->writingSystemComboBox;
}

void QQuickFontDialogImplAttached::setWritingSystemComboBox(QQuickComboBox *writingSystemComboBox)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->writingSystemComboBox == writingSystemComboBox)
        return;

    if (d->writingSystemComboBox) {
        disconnect(d->writingSystemComboBox, &QQuickComboBox::activated,
                   this, &QQuickFontDialogImplAttached::_q_writingSystemChanged);
    }

    d->writingSystemComboBox = writingSystemComboBox;

    if (d->writingSystemComboBox) {
        QStringList writingSystemModel;
        for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
            QFontDatabase::WritingSystem ws = QFontDatabase::WritingSystem(i);
            QString wsName = QFontDatabase::writingSystemName(ws);
            if (wsName.isEmpty())
                break;
            writingSystemModel.append(wsName);
        }

        d->writingSystemComboBox->setModel(writingSystemModel);

        connect(d->writingSystemComboBox, &QQuickComboBox::activated,
                this, &QQuickFontDialogImplAttached::_q_writingSystemChanged);
    }

    emit writingSystemComboBoxChanged();
}

QQuickCheckBox *QQuickFontDialogImplAttached::underlineCheckBox() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->underlineCheckBox;
}

void QQuickFontDialogImplAttached::setUnderlineCheckBox(QQuickCheckBox *underlineCheckBox)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->underlineCheckBox == underlineCheckBox)
        return;

    if (d->underlineCheckBox) {
        disconnect(d->underlineCheckBox, &QQuickCheckBox::checkStateChanged,
                   this, &QQuickFontDialogImplAttached::_q_updateSample);
    }

    d->underlineCheckBox = underlineCheckBox;

    if (d->underlineCheckBox) {
        connect(d->underlineCheckBox, &QQuickCheckBox::checkStateChanged,
                this, &QQuickFontDialogImplAttached::_q_updateSample);
    }

    emit underlineCheckBoxChanged();
}

QQuickCheckBox *QQuickFontDialogImplAttached::strikeoutCheckBox() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->strikeoutCheckBox;
}

void QQuickFontDialogImplAttached::setStrikeoutCheckBox(QQuickCheckBox *strikeoutCheckBox)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->strikeoutCheckBox == strikeoutCheckBox)
        return;

    if (d->strikeoutCheckBox) {
        disconnect(d->strikeoutCheckBox, &QQuickCheckBox::checkStateChanged,
                   this, &QQuickFontDialogImplAttached::_q_updateSample);
    }

    d->strikeoutCheckBox = strikeoutCheckBox;

    if (d->strikeoutCheckBox) {
        connect(d->strikeoutCheckBox, &QQuickCheckBox::checkStateChanged,
                this, &QQuickFontDialogImplAttached::_q_updateSample);
    }

    emit strikeoutCheckBoxChanged();
}

QQuickTextField *QQuickFontDialogImplAttached::familyEdit() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->familyEdit;
}

void QQuickFontDialogImplAttached::setFamilyEdit(QQuickTextField *familyEdit)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->familyEdit == familyEdit)
        return;

    d->familyEdit = familyEdit;

    emit familyEditChanged();
}

QQuickTextField *QQuickFontDialogImplAttached::styleEdit() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->styleEdit;
}

void QQuickFontDialogImplAttached::setStyleEdit(QQuickTextField *styleEdit)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->styleEdit == styleEdit)
        return;

    d->styleEdit = styleEdit;

    emit styleEditChanged();
}

QQuickTextField *QQuickFontDialogImplAttached::sizeEdit() const
{
    Q_D(const QQuickFontDialogImplAttached);
    return d->sizeEdit;
}

void QQuickFontDialogImplAttached::setSizeEdit(QQuickTextField *sizeEdit)
{
    Q_D(QQuickFontDialogImplAttached);

    if (d->sizeEdit == sizeEdit)
        return;

    if (d->sizeEdit) {
        disconnect(d->sizeEdit, &QQuickTextField::textChanged,
                   this, &QQuickFontDialogImplAttached::_q_sizeEdited);
    }

    d->sizeEdit = sizeEdit;

    if (d->sizeEdit) {
        connect(d->sizeEdit, &QQuickTextField::textChanged,
                this, &QQuickFontDialogImplAttached::_q_sizeEdited);
    }

    emit sizeEditChanged();
}

static int findFamilyInModel(const QString &selectedFamily, const QStringList &model)
{
    enum match_t { MATCH_NONE = 0, MATCH_LAST_RESORT = 1, MATCH_APP = 2, MATCH_FAMILY = 3 };
    QString foundryName1, familyName1, foundryName2, familyName2;
    int bestFamilyMatch = -1;
    match_t bestFamilyType = MATCH_NONE;
    const QFont f;

    QFontDatabasePrivate::parseFontName(selectedFamily, foundryName1, familyName1);

    int i = 0;
    for (auto it = model.constBegin(); it != model.constEnd(); ++it, ++i) {
        QFontDatabasePrivate::parseFontName(*it, foundryName2, familyName2);

        if (familyName1 == familyName2) {
            bestFamilyType = MATCH_FAMILY;
            if (foundryName1 == foundryName2)
                return i;
            else
                bestFamilyMatch = i;
        }

        // fallbacks
        match_t type = MATCH_NONE;
        if (bestFamilyType <= MATCH_NONE && familyName2 == QStringLiteral("helvetica"))
            type = MATCH_LAST_RESORT;
        if (bestFamilyType <= MATCH_LAST_RESORT && familyName2 == f.families().constFirst())
            type = MATCH_APP;
        if (type != MATCH_NONE) {
            bestFamilyType = type;
            bestFamilyMatch = i;
        }
    }

    return bestFamilyMatch;
}

static int findStyleInModel(const QString &selectedStyle, const QStringList &model)
{
    if (model.isEmpty())
        return -1;

    if (!selectedStyle.isEmpty()) {
        const int idx = model.indexOf(QRegularExpression(QRegularExpression::escape(selectedStyle), QRegularExpression::CaseInsensitiveOption));
        if (idx >= 0)
            return idx;

        enum class StyleClass {Unknown, Normal, Italic};
        auto classifyStyleFallback = [](const QString & style) {
            if (style.toLower() == QLatin1String("italic") || style.toLower() == QLatin1String("oblique"))
                return StyleClass::Italic;
            if (style.toLower() == QLatin1String("normal") || style.toLower() == QLatin1String("regular"))
                return StyleClass::Normal;
            return StyleClass::Unknown;
        };

        auto styleClass = classifyStyleFallback(selectedStyle);

        if (styleClass != StyleClass::Unknown)
            for (int i = 0; i < model.size(); ++i)
                if (classifyStyleFallback(model.at(i)) == styleClass)
                    return i;
    }
    return 0;
}

/*!
    \internal

    Updates the model for the family list view, and attempt
    to reselect the previously selected font family.
 */
void QQuickFontDialogImplAttached::updateFamilies()
{
    const QFontDialogOptions::FontDialogOptions scalableMask(
            QFontDialogOptions::ScalableFonts | QFontDialogOptions::NonScalableFonts);

    const QFontDialogOptions::FontDialogOptions spacingMask(QFontDialogOptions::ProportionalFonts
                                                            | QFontDialogOptions::MonospacedFonts);

    const auto p = qobject_cast<QQuickFontDialogImpl *>(parent());

    const auto options = p->options()->options();

    QStringList familyNames;
    const auto families = QFontDatabase::families(m_writingSystem);
    for (const auto &family : families) {
        if (QFontDatabase::isPrivateFamily(family))
            continue;

        if ((options & scalableMask) && (options & scalableMask) != scalableMask) {
            if (bool(options & QFontDialogOptions::ScalableFonts)
                != QFontDatabase::isSmoothlyScalable(family))
                continue;
        }

        if ((options & spacingMask) && (options & scalableMask) != spacingMask) {
            if (bool(options & QFontDialogOptions::MonospacedFonts)
                != QFontDatabase::isFixedPitch(family))
                continue;
        }

        familyNames << family;
    }

    auto listView = familyListView();

    // Index will be set to -1 on empty model, and 0 for non empty models.
    m_ignoreFamilyUpdate = !m_selectedFamily.isEmpty();
    listView->setModel(familyNames);
    m_ignoreFamilyUpdate = false;

    // Will overwrite selectedFamily and selectedStyle
    listView->setCurrentIndex(findFamilyInModel(m_selectedFamily, familyNames));

    if (familyNames.isEmpty())
        _q_familyChanged();
}

/*!
    \internal

    Updates the model for the style list view, and
    attempt to reselect the style that was previously selected.

    Calls updateSizes()
 */
void QQuickFontDialogImplAttached::updateStyles()
{
    const QString family = familyListView()->currentIndex() >= 0 ? m_selectedFamily : QString();
    const QStringList styles = QFontDatabase::styles(family);

    auto listView = styleListView();

    m_ignoreStyleUpdate = !m_selectedStyle.isEmpty();
    listView->setModel(styles);

    if (styles.isEmpty()) {
        styleEdit()->clear();
        m_smoothlyScalable = false;
    } else {
        int newIndex = findStyleInModel(m_selectedStyle, styles);

        listView->setCurrentIndex(newIndex);

        m_selectedStyle = styles.at(newIndex);
        styleEdit()->setText(m_selectedStyle);

        m_smoothlyScalable = QFontDatabase::isSmoothlyScalable(m_selectedFamily, m_selectedStyle);
    }

    m_ignoreStyleUpdate = false;

    updateSizes();
}

/*!
    \internal

    Updates the model for the size list view,
    and attempts to reselect the size that was previously selected
 */
void QQuickFontDialogImplAttached::updateSizes()
{
    if (!m_selectedFamily.isEmpty()) {
        const QList<int> sizes = QFontDatabase::pointSizes(m_selectedFamily, m_selectedStyle);

        QStringList str_sizes;

        str_sizes.reserve(sizes.size());

        int idx = 0, current = -1;
        for (QList<int>::const_iterator it = sizes.constBegin(); it != sizes.constEnd(); it++) {
            str_sizes.append(QString::number(*it));
            if (current == -1 && m_selectedSize == *it) {
                current = idx;
            }
            ++idx;
        }

        auto listView = sizeListView();

        // only select the first element in the model when this function is first called and the new model isn't empty
        listView->setModel(str_sizes);

        if (current != -1)
            listView->setCurrentIndex(current);

        sizeEdit()->setText(!m_smoothlyScalable && listView->currentIndex() > 0
                                    ? str_sizes.at(listView->currentIndex())
                                    : QString::number(m_selectedSize));
    } else {
        qCWarning(lcAttachedProperty) << "Warning! selectedFamily is empty";
        sizeEdit()->clear();
    }

    _q_updateSample();
}

void QQuickFontDialogImplAttached::_q_updateSample()
{
    if (m_selectedFamily.isEmpty())
        return;

    const int pSize = sizeEdit()->text().toInt();

    QFont newFont = QFontDatabase::font(m_selectedFamily, m_selectedStyle, pSize);

    newFont.setUnderline(underlineCheckBox()->isChecked());
    newFont.setStrikeOut(strikeoutCheckBox()->isChecked());

    sampleEdit()->setFont(newFont);
}

void QQuickFontDialogImplAttached::_q_writingSystemChanged(int index)
{
    m_writingSystem = QFontDatabase::WritingSystem(index);
    sampleEdit()->setText(QFontDatabase::writingSystemSample(m_writingSystem));

    updateFamilies();
}

void QQuickFontDialogImplAttached::searchListView(const QString &s, QQuickListView *listView)
{
    if (s.isEmpty())
        return;

    const QStringList model = listView->model().toStringList();

    bool redo = false;

    do {
        m_search.append(s);

        for (int i = 0; i < model.size(); ++i) {
            if (model.at(i).startsWith(m_search, Qt::CaseInsensitive)) {
                listView->setCurrentIndex(i);
                return;
            }
        }

        clearSearch();

        redo = !redo;
    } while (redo);
}

void QQuickFontDialogImplAttached::clearSearch()
{
    m_search.clear();
}

void QQuickFontDialogImplAttached::_q_familyChanged()
{
    if (m_ignoreFamilyUpdate)
        return;

    const int index = familyListView()->currentIndex();

    if (index < 0) {
        familyEdit()->clear();
    } else {
        m_selectedFamily = familyListView()->model().toStringList().at(index);
        familyEdit()->setText(m_selectedFamily);
    }

    updateStyles();
}

void QQuickFontDialogImplAttached::_q_styleChanged()
{
    if (m_ignoreStyleUpdate)
        return;

    const int index = styleListView()->currentIndex();

    if (index < 0) {
        qCWarning(lcAttachedProperty) << "currentIndex changed to -1";
        return;
    }

    m_selectedStyle = styleListView()->model().toStringList().at(index);
    styleEdit()->setText(m_selectedStyle);
    m_smoothlyScalable = QFontDatabase::isSmoothlyScalable(m_selectedFamily, m_selectedStyle);

    updateSizes();
}

void QQuickFontDialogImplAttached::_q_sizeEdited()
{
    const int size = qAbs(sizeEdit()->text().toInt());

    if (size == m_selectedSize)
        return;

    m_selectedSize = size;

    if (sizeListView()->count()) {
        auto model = sizeListView()->model().toStringList();

        int i;
        for (i = 0; i < model.size() - 1; ++i) {
            if (model.at(i).toInt() >= size)
                break;
        }

        QSignalBlocker blocker(sizeListView());
        if (model.at(i).toInt() == size)
            sizeListView()->setCurrentIndex(i);
        else
            sizeListView()->setCurrentIndex(-1);
    }

    _q_updateSample();
}

void QQuickFontDialogImplAttached::_q_sizeChanged()
{
    const int index = sizeListView()->currentIndex();

    if (index < 0) {
        qCWarning(lcAttachedProperty) << "currentIndex changed to -1";
        return;
    }

    const QString s = sizeListView()->model().toStringList().at(index);
    m_selectedSize = s.toInt();

    sizeEdit()->setText(s);

    _q_updateSample();
}

void QQuickFontDialogImplAttachedPrivate::currentFontChanged(const QFont &font)
{
    auto fontDialogImpl = qobject_cast<QQuickFontDialogImpl *>(parent);

    if (!fontDialogImpl) {
        return;
    }

    fontDialogImpl->setCurrentFont(font);
}

void QQuickFontDialogImplAttached::selectFontInListViews(const QFont &font)
{
    {
        QSignalBlocker blocker(sampleEdit());
        familyListView()->setCurrentIndex(findFamilyInModel(font.families().constFirst(), familyListView()->model().toStringList()));
        styleListView()->setCurrentIndex(findStyleInModel(QFontDatabase::styleString(font), styleListView()->model().toStringList()));
        sizeEdit()->setText(QString::number(font.pointSize()));

        underlineCheckBox()->setChecked(font.underline());
        strikeoutCheckBox()->setChecked(font.strikeOut());
    }

    _q_updateSample();
}

QT_END_NAMESPACE

#include "moc_qquickfontdialogimpl_p.cpp"
