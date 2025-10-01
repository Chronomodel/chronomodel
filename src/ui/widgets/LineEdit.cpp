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

LineEdit::LineEdit(QWidget* parent):
    QLineEdit(parent),
    mAdjustText(true)
{
    setParent(parent);
    setPalette(parent->palette());

    setAlignment(Qt::AlignHCenter);

    if (parentWidget()) {
        setFont(parentWidget()->font());
    }
}

void LineEdit::resetText(double value)
{
    QLineEdit::blockSignals(true);
    //QString text = QLocale().toString(value, 'g', std::numeric_limits<double>::digits10 + 1); // Définit la précision au maximum de digits possibles pour un double
    QString text;

    if (value == 0.0) {
        // Pour 0 On force la notation décimale avec 3 décimales en utilisant la locale
        text = QLocale().toString(0.0, 'f', 3);

    } else if (std::abs(value) < 1.0) {
        // Notation scientifique avec 3 décimales en utilisant la locale
        text = QLocale().toString(value, 'e', 3);
    } else {
        // Notation décimale avec 3 décimales en utilisant la locale
        text = QLocale().toString(value, 'f', 3);
    }
    QLineEdit::setText(text);
    QLineEdit::blockSignals(false);
};

void LineEdit::resetText(const QString& text)
{
    QLineEdit::blockSignals(true);
    QLineEdit::setText(text);
    QLineEdit::blockSignals(false);
};

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
}
void LineEdit::show()
{
    QWidget::show();
}

void LineEdit::hide()
{
    QWidget::hide();
}


void LineEdit::setAdjustText(bool ad)
{
    mAdjustText = ad;
}

void LineEdit::adjustFont()
{
    if (!text().isEmpty() && mAdjustText) {
        const QFontMetrics fm (qApp->font());
        const QRect textRect = fm.boundingRect(text());
        const qreal wR = width() - 10;

        QFont ft = qApp->font();
        if (wR>0) {
            const qreal xfactor = (textRect.width() > wR) ? textRect.width()/wR : 1;
            const qreal yfactor = (height() && (textRect.height() > height())) ? textRect.height()/height() : 1;
            const qreal factor = (xfactor > yfactor) ? xfactor : yfactor;

            ft.setPointSizeF(ft.pointSizeF()/factor);

        } else {
            ft.setPointSizeF(1);

        }
        setFont(ft);

    }
}

void LineEdit::resizeEvent(QResizeEvent* e)
{
    (void) e;
    adjustFont();
}

void LineEdit::setFont(const QFont& font)
{
#ifdef Q_OS_MAC
    QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    QLineEdit::setStyleSheet(styleSh);
#elif defined(Q_OS_WIN)
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    QLineEdit::setFont(font);
#else
    QString styleSh = "QLineEdit { border-radius: 5px; font: "+ QString::number(font.pointSize()) + "px ;font-family: "+font.family() + ";}";
    QLineEdit::setStyleSheet(styleSh);
#endif

}

LineEdit::~LineEdit()
{

}
