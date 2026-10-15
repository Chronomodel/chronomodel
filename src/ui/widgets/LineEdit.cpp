/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#include "LineEdit.h"
#include <QFont>
#include <QGuiApplication>
#include <QLocale>
#include <QResizeEvent>

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent),
    mAdjustText(true)
{
    setAlignment(Qt::AlignHCenter);

    // Police de base du widget ou du parent
    if (parentWidget())
        mBaseFont = parentWidget()->font();
    else
        mBaseFont = font();

    setFont(mBaseFont);

    // Appliquer un style une seule fois (bord arrondi)
    setStyleSheet("QLineEdit { border-radius: 5px; }");

    // Ajustement automatique du texte à chaque modification
    connect(this, &QLineEdit::textChanged, this, &LineEdit::adjustFont);
}

LineEdit::~LineEdit() = default;

void LineEdit::resetText(double value)
{
    {
        const QSignalBlocker blocker(QLineEdit);

        static const QLocale locale;
        QString text;

        if (value == 0.0)
            text = locale.toString(0.0, 'f', 3);
        else if (std::abs(value) < 0.01)
            text = locale.toString(value, 'e', 3);
        else
            text = locale.toString(value, 'f', 3);

        QLineEdit::setText(text);

    }
    adjustFont();
}

void LineEdit::resetText(const QString& text)
{
    {
        const QSignalBlocker blocker(QLineEdit);
        QLineEdit::setText(text);
    }

    adjustFont();
}

void LineEdit::setAdjustText(bool enable)
{
    mAdjustText = enable;
    if (enable) adjustFont();
}

void LineEdit::adjustFont()
{
    if (!mAdjustText || text().isEmpty() || width() <= 0 || height() <= 0)
        return;

    QFont ft = mBaseFont; // toujours partir de la police originale
    QFontMetrics fm(ft);

    const int textWidth  = fm.horizontalAdvance(text());
    const int textHeight = fm.height();
    const int availWidth = width() - 2;   // marge interne
    const int availHeight= height() - 2;  // marge interne

    if (textWidth > availWidth || textHeight > availHeight) {
        qreal xFactor = static_cast<qreal>(textWidth) / availWidth;
        qreal yFactor = static_cast<qreal>(textHeight) / availHeight;
        qreal factor = std::max(xFactor, yFactor);

        ft.setPointSizeF(ft.pointSizeF() / factor);
    }

    QLineEdit::setFont(ft); // applique la police calculée
}

void LineEdit::resizeEvent(QResizeEvent* event)
{
    QLineEdit::resizeEvent(event);
    adjustFont(); // ajuste la police automatiquement à chaque redimensionnement
}

void LineEdit::setFont(const QFont &font)
{
    mBaseFont = font;        // mettre à jour la police de référence
    QLineEdit::setFont(font); // appliquer au widget
    adjustFont();             // ajuster immédiatement si nécessaire
}

void LineEdit::show()
{
    QWidget::show();
    adjustFont();
}

void LineEdit::hide()
{
    QWidget::hide();
}

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        adjustFont(); // ajuste la police quand le widget devient visible
}

