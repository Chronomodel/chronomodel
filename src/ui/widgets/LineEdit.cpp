/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2025

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
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
    QLineEdit::blockSignals(true);
    static const QLocale locale;
    QString text;

    if (value == 0.0)
        text = locale.toString(0.0, 'f', 3);
    else if (std::abs(value) < 0.01)
        text = locale.toString(value, 'e', 3);
    else
        text = locale.toString(value, 'f', 3);

    QLineEdit::setText(text);
    QLineEdit::blockSignals(false);

    adjustFont();
}

void LineEdit::resetText(const QString& text)
{
    QLineEdit::blockSignals(true);
    QLineEdit::setText(text);
    QLineEdit::blockSignals(false);

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
    const int availWidth = width() - 4;   // marge interne
    const int availHeight= height() - 4;  // marge interne

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

