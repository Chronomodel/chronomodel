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

#include "ScrollCompressor.h"
#include "Painting.h"

ScrollCompressor::ScrollCompressor(QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mProp(0.5),
mIsDragging(false),
mShowText(true),
mIsVertical(true)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

ScrollCompressor::~ScrollCompressor()
{

}

QSize ScrollCompressor::sizeHint() const
{
    return mIsVertical ? QSize(20, 100) : QSize(100, 20);
}

void ScrollCompressor::setVertical(bool vertical)
{
    mIsVertical = vertical;
    if (mIsVertical)
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    else
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    update();
}

void ScrollCompressor::setProp(const double& prop, bool sendNotification)
{
    mProp = prop;
    mProp = (mProp < 0.) ? 0. : mProp;
    mProp = (mProp > 1.) ? 1. : mProp;
    if (sendNotification)
        emit valueChanged(mProp);
    update();
}

double ScrollCompressor::getProp() const
{
    return mProp;
}

void ScrollCompressor::showText(const QString& text, bool show)
{
    mShowText = show;
    mText = text;
    update();
}

void ScrollCompressor::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QRectF r = rect();//.adjusted(1, 1, -1, -1);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QFont adaptedFont (font());
    QFontMetricsF fm (adaptedFont);
    qreal textSize = fm.horizontalAdvance(mText);
    if (!mIsVertical && textSize > (r.width() - 2. ) ) {
        const qreal fontRate = textSize / (r.width() - 2. );
        const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
        adaptedFont.setPointSizeF(ptSiz);
    }
    p.setFont(adaptedFont);

    if (mIsVertical) {
        QLinearGradient grad1(0, 0, width(), 0);
        grad1.setColorAt(0, QColor(60, 60, 60));
        grad1.setColorAt(1, QColor(80, 80, 80));

        p.setPen(Qt::NoPen);
        p.setBrush(grad1);
        p.drawRect(r);

        QLinearGradient grad2(0, 0, width(), 0);
        grad2.setColorAt(0, Painting::mainColorLight);
        grad2.setColorAt(1, Painting::mainColorDark);

        qreal h = r.height() * mProp;
        QRectF r2 = r.adjusted(0, r.height() - h, 0, 0);
        p.fillRect(r2, grad2);

        p.setPen(Painting::borderDark);
        p.setBrush(Qt::NoBrush);
        p.drawRect(r);

        if (mShowText) {
           // QString text = mText;// + "\r" + QString::number(qRound(mProp * 100)) + " %";
            p.setPen(QColor(200, 200, 200));
            p.rotate(-90); // coordonates are rotated
            const int x = (- r.height() - fm.horizontalAdvance(mText))/2;
            const int y = r.width()/2;
            p.drawText(x, y, mText);
            p.rotate(90);
        }
    } else {
        const qreal w = r.width() * mProp;
        const QRectF r2 = r.adjusted(0, 0, -r.width() + w, 0);

        p.setPen(QColor(150, 150, 150));
        p.setBrush(QColor(150, 150, 150));
        p.drawRect(r);

        p.setPen(Painting::mainGreen);
        p.setBrush(Painting::mainGreen);
        p.drawRect(r2);

        if (mShowText) {
            QString text = mText + " : " + QString::number(qRound(mProp * 100)) + " %";
            p.setPen(Qt::white);
            p.drawText(r, Qt::AlignCenter, text);
        }
    }
}

void ScrollCompressor::mousePressEvent(QMouseEvent* e)
{
    updateProp(e);
    mIsDragging = true;
}

void ScrollCompressor::mouseReleaseEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    mIsDragging = false;
}

void ScrollCompressor::mouseMoveEvent(QMouseEvent* e)
{
    if (mIsDragging)
        updateProp(e);

}

void ScrollCompressor::mouseDoubleClickEvent(QMouseEvent *e)
{
    (void) e;
    mProp = 0.5;
    emit valueChanged(mProp);
    update();
}

void ScrollCompressor::updateProp(QMouseEvent* e)
{
    QRect r = rect();
    if (mIsVertical) {
        int y = r.height() - e->pos().y();
        y = (y < 0) ? 0 : y;
        y = (y > r.height()) ? r.height() : y;
        mProp = double(y) / double (r.height());

    } else {
        int x = e->pos().x();
        x = (x < 0) ? 0 : x;
        x = (x > r.width()) ? r.width() : x;
        mProp = double(x) / double(r.width());
    }
    emit valueChanged(mProp);
    update();
}
