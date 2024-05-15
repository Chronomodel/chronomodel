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

#include "Label.h"
#include "Painting.h"

Label::Label(QWidget* parent):QLabel(parent),
    mIsTitle(false),
mAdjustText(true)
{
    init();
}

Label::Label(const QString& text, QWidget* parent):QLabel(text, parent),
    mIsTitle(false)
{
    init();
}

Label::~Label()
{

}
void Label::adjustFont()
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

void Label::init()
{
    setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mPalette = qApp->palette();
    setFont(qApp->font());
}

void Label::setPalette(QPalette &palette)
{
    mPalette = palette;
}

void Label::setBackground(QColor color)
{
    mPalette.setColor(QPalette::Window, color);
}

void Label::setIsTitle(bool isTitle)
{
    mIsTitle = isTitle;

    if (mIsTitle) {
        mPalette.setColor(QPalette::Text, Qt::white);
        mPalette.setColor(QPalette::Window, Painting::mainColorGrey);
        setAlignment(Qt::AlignCenter);
     }
    else {
        mPalette = qApp->palette();
    }
}

void Label::setAdjustText(bool ad)
{
    mAdjustText = ad;
}
void Label::resizeEvent(QResizeEvent* e)
{
    (void) e;
    adjustFont();

}

void Label::paintEvent(QPaintEvent*)
{
   // adjustFont();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font());
    const QRectF r = rect();

    p.fillRect(r, mPalette.window().color());
    p.setPen(mPalette.text().color());

    p.drawText(r, alignment(), text());

}


void Label::setLight()
{
    mPalette.setColor(QPalette::Text, QColor(200, 200, 200));
}

void Label::setDark()
{
    mPalette.setColor(QPalette::Text, Qt::black);
}
