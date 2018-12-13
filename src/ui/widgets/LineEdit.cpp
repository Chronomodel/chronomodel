/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

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
#include "Painting.h"

#include <QFont>
#include <QtWidgets>

LineEdit::LineEdit(QWidget* parent):QLineEdit(parent),
  mAdjustText(true)
{
    setParent(parent);
    setAlignment(Qt::AlignHCenter);
    setFont(parentWidget()->font());
}

void LineEdit::setVisible(bool visible)
{
    QWidget::setVisible(visible);
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
        const qreal xfactor (textRect.width()> wR ? textRect.width()/wR : 1);
        const qreal yfactor (textRect.height()>height() ? textRect.height()/height() : 1) ;
        const qreal factor  = ( xfactor > yfactor ? xfactor : yfactor);
        QFont ft = qApp->font();
        ft.setPointSizeF(ft.pointSizeF()/factor);
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
#else
//#ifdef Q_OS_WIN
    QWidget::setStyleSheet("QLineEdit { border-radius: 5px;}");
    QLineEdit::setFont(font);
 #endif
}

LineEdit::~LineEdit()
{

}
