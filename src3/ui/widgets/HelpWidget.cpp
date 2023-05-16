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

#include "HelpWidget.h"
#include "Painting.h"

HelpWidget::HelpWidget(QWidget* parent):QWidget(parent)
{
    construct();
}

HelpWidget::HelpWidget(const QString& text, QWidget* parent):QWidget(parent)
{
    construct();
    setText(text);
}

void HelpWidget::construct()
{
    mFont = font();

    mHyperLink = new QLabel(this);
    mHyperLink->setTextFormat(Qt::RichText);
    mHyperLink->setFont(mFont);
    mHyperLink->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    mHyperLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    mHyperLink->setOpenExternalLinks(true);

    // Not yet supported with retina display in Qt 5.3
#ifndef Q_OS_MAC
   /* QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setColor(Qt::black);
    shadow->setBlurRadius(4);
    shadow->setOffset(1, 1);
    setGraphicsEffect(shadow);
    */
#endif
}

HelpWidget::~HelpWidget()
{

}

void HelpWidget::setText(const QString& text)
{
    mText = text;
    update();
}
void HelpWidget::setLink(const QString& url)
{
    mHyperLink->setText("<a href=\"" + url + "\">More...</a>");
}

int HelpWidget::heightForWidth(int w) const
{
    QFontMetrics metrics(mFont);
    QRect rect = metrics.boundingRect(QRect(0, 0, w - 10, 1000),
                                      Qt::TextWordWrap | Qt::AlignVCenter | Qt::AlignLeft,
                                      mText);
    return rect.height() + 10 + 5 + mFont.pointSize(); // 15 is the height of the link, and 5 its margin
}

void HelpWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QRectF r = rect().adjusted(1, 1, -1, -1);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QColor(187, 174, 105));
    p.setBrush(QColor(242, 238, 184));
    p.drawRoundedRect(r, 4, 4);

    p.setPen(QColor(104, 74, 64));
    p.setFont(mFont);
    QTextOption options;
    options.setWrapMode(QTextOption::WordWrap);
    options.setAlignment(Qt::AlignLeft);// | Qt::AlignVCenter);
    p.drawText(rect().adjusted(5, 5, -5, -5), mText, options);
}

void HelpWidget::resizeEvent(QResizeEvent*)
{
    QFontMetrics metrics(mFont);
    mHyperLink->setFont(mFont);
    mHyperLink->setGeometry(5, height() - 2*mFont.pointSize() -5, width() - metrics.boundingRect("More..").width() -5, 2*mFont.pointSize());
}
