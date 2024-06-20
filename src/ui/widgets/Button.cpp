/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#include "Button.h"
#include "Painting.h"
#include "AppSettings.h"

#include <QtWidgets>


Button::Button(QWidget* parent):
    QPushButton(parent),
    mFlatVertical(false),
    mFlatHorizontal(false),
    mIsClose (false),
    mIconOnly(true),
    mMouseOver(false),
    mColorState(eDefault),
    mFont(QPushButton::font()),
    mUseMargin (false)
{
    setAutoRepeat(false);
    setCursor(Qt::PointingHandCursor);

    QPushButton::setFlat(false);
    //setPalette(parent->palette());

}

Button::Button(const QString &text, QWidget* parent):QPushButton(text, parent),
    mFlatVertical(false),
    mFlatHorizontal(false),
    mIsClose (false),
    mIconOnly(true),
    mMouseOver(false),
    mColorState(eDefault),
    mFont(QPushButton::font()),
    mUseMargin (false)
{
    setAutoRepeat(false);
    setCursor(Qt::PointingHandCursor);
    QPushButton::setFlat(false);

    QIcon ic = icon();
    bool textOnly = !mIconOnly && !text.isEmpty() && ic.isNull();

    if (!mIconOnly) {
        mFont = QPushButton::font();
        if (textOnly)
            mFont.setBold(true);

        QFontMetricsF fm (mFont);
        qreal textSize = fm.horizontalAdvance(text);

        while (textSize > (rect().width() - 30. )) {
            mFont.setPointSizeF(mFont.pointSizeF() - 1);
            fm = QFontMetricsF(mFont);
            textSize = fm.horizontalAdvance(text);
        }
    }


}

Button::Button(const Button &button, QWidget* parent):
    QPushButton(button.text(), parent),
    mFlatVertical (button.mFlatVertical),
    mFlatHorizontal (button.mFlatHorizontal),
    mIsClose (button.mIsClose),
    mIconOnly(true),
    mMouseOver (button.mMouseOver),
    mColorState(button.mColorState),
    mFont(button.mFont),
    mUseMargin(button.mUseMargin)
{
    QPushButton::setFlat(button.isFlat());
    setIcon(button.icon());
    setToolTip(button.toolTip() );
    setAutoRepeat(button.autoRepeat());
    setCursor(button.cursor());
}

/* void Button::init()
{
    setAutoRepeat(false);
    setCursor(Qt::PointingHandCursor);

    mMouseOver = false;
    mFlatVertical = false;
    mFlatHorizontal = false;
    QPushButton::setFlat(false);
    mIsClose = false;

    mColorState = eDefault;

    mUseMargin = false;


    QIcon ic = icon();
    bool textOnly = !mIconOnly && !text().isEmpty() && ic.isNull();

    if (!mIconOnly) {
        mFont = QPushButton::font();
        if (textOnly)
            mFont.setBold(true);

        QFontMetricsF fm (mFont);
        qreal textSize = fm.horizontalAdvance(text());

        while (textSize > (rect().width() - 30. )) {
            mFont.setPointSizeF(mFont.pointSizeF() - 1);
            fm = QFontMetricsF(mFont);
            textSize = fm.horizontalAdvance(text());
        }
    }

  
}
*/

Button::~Button()
{
}
void Button::keyPressEvent(QKeyEvent* event)
{
    QPushButton::keyPressEvent(event);
}

void Button::setFlatVertical()
{
    mFlatVertical = true;
    QPushButton::setFlat(true);
    update();
}

void Button::setFlatHorizontal()
{
    mFlatHorizontal = true;
    QPushButton::setFlat(true);
    update();
}

void Button::setIsClose(bool isClose)
{
    mIsClose = isClose;
    update();
}

void Button::setColorState(ColorState state)
{
    mColorState = state;
    update();
}

bool Button::event(QEvent* event)
{
    return QPushButton::event(event);
}

void Button::enterEvent(QEnterEvent *e)
{
    mMouseOver = true;
    update();
    QPushButton::QWidget::enterEvent(e);
}

void Button::leaveEvent(QEvent * e)
{
    mMouseOver = false;
    update();
    QPushButton::QWidget::leaveEvent(e);
}

void Button::resizeEvent(QResizeEvent* e)
{
    const bool textOnly = !mIconOnly && !text().isEmpty() && icon().isNull();

    if (!mIconOnly) {
        mFont = QPushButton::font();
        if (textOnly)
            mFont.setBold(true);

        QFontMetricsF fm (mFont);
        qreal textSize = fm.horizontalAdvance(text());

        while (textSize > (rect().width() - 5. )) {
            mFont.setPointSizeF(mFont.pointSizeF() - 1);
            fm = QFontMetricsF(mFont);
            textSize = fm.horizontalAdvance(text());
        }
    }

    QPushButton::resizeEvent(e);
}
void Button::setCheckState(const bool checkState)
{
    blockSignals(true);
    setChecked(checkState);
    blockSignals(false);

    if (!checkState)
        setColorState(eWarning);
    else
        setColorState(eDefault);
}

void Button::setCheckable(const bool checkable)
{
    if (!checkable)
        mColorState = eWarning;
    else
        mColorState = eDefault;

    QPushButton::setCheckable(checkable);

}

void Button::setText(const QString& text)
{
    QPushButton::setText(text);
}

void Button::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect();

    if (mUseMargin) {
#ifdef Q_OS_MAC
        const int m = style()->pixelMetric(QStyle::PM_ButtonMargin);
        r.adjust(m, m, -m, -m);
#endif
    }
    if (mIsClose) {
        r.adjust(1, 1, -1, -1);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        painter.drawEllipse(r);

        int m1 = 3;
        int m2 = 6;

        painter.setBrush(Qt::black);
        painter.drawEllipse(r.adjusted(m1, m1, -m1, -m1));

        painter.save();

        painter.translate(r.x() + r.width()/2, r.y() + r.height()/2);
        painter.rotate(45.);
        QPen pen;
        pen.setColor(Qt::white);
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidthF(3.);
        painter.setPen(pen);

        int s = r.width()/2 - m2;

        painter.drawLine(0, -s, 0, s);
        painter.drawLine(-s, 0, s, 0);

        painter.restore();

    } else if (mFlatVertical || mFlatHorizontal) {

        //QColor gradColTop(40, 40, 40);
        //QColor gradColTop(180, 180, 180);
        QColor gradColTop = Painting::borderDark;
        QColor gradColBot(30, 30, 30);
        QColor gradLineLight(Painting::borderDark);
        QColor gradLineDark(0, 0, 0);

        if (!isEnabled()) {
            gradColTop = QColor(110, 110, 110);
            gradColBot = QColor(100, 100, 100);
            gradLineLight = QColor(120, 120, 120);
            gradLineDark = QColor(80, 80, 80);

        } else if (isDown() || isChecked()) {
            gradColTop = Painting::mainColorDark;
            gradColBot = Painting::mainColorDark.lighter(225);
            gradLineLight = QColor(30, 30, 30);
            gradLineDark = QColor(10, 10, 10);
        }
/*
        QGradient grad;
        if (mFlatVertical) {
           grad =  QLinearGradient(0, 0, r.width(), r.height());
           grad.setColorAt(0., gradColBot);
           grad.setColorAt(0.85, gradColBot);
           grad.setColorAt(1, gradColTop);
*/
          /* grad = QRadialGradient(QPointF(r.width()/2, 0),  3*r.height()/3);//QLinearGradient (0, 0, 0, r.height());
          // grad.setColorAt(0., gradColBot);
           grad.setColorAt(0.85, gradColBot);
           grad.setColorAt(1, gradColTop);*/
 /*       } else if (mFlatHorizontal) {
           grad = QRadialGradient(QPointF(r.width()/2, 0),  2*r.width()/3);//QLinearGradient (0, 0, 0, r.height());
           grad.setColorAt(0., gradColBot);
           grad.setColorAt(0.5, gradColBot);
           grad.setColorAt(1, gradColTop);
        }
*/
       // grad.setColorAt(0, gradColTop);


        if (mMouseOver)
            //painter.fillRect(r.adjusted(-50, -50, 50, 50), grad);
            painter.fillRect(r.adjusted(-50, -50, 50, 50), gradColTop);
        else
            painter.fillRect(r, gradColTop);
/*
        painter.setPen(gradLineLight);
        if (mFlatVertical)
            painter.drawLine(0, 0, r.width(), 0);

        else if (mFlatHorizontal)
            painter.drawLine(0, 0, 0, r.height());

        painter.setPen(gradLineDark);
        if (mFlatVertical)
            painter.drawLine(0, r.height(), r.width(), r.height());

        else if (mFlatHorizontal)
            painter.drawLine(r.width(), 0, r.width(), r.height());
*/
        // ---------

        QIcon ic = icon();
        painter.setPen(QColor(200, 200, 200));


        bool textOnly = !mIconOnly && !text().isEmpty() && ic.isNull();


        painter.setFont(mFont);
        if (textOnly) {
            if (mMouseOver) {
                 painter.setPen(QColor(250, 250, 250));
                 painter.drawText(r.adjusted(-30, -30, 30, 30), Qt::AlignHCenter | Qt::AlignVCenter, text());

            } else {
                painter.setPen(QColor(200, 200, 200));
                painter.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, text());
            }

        } else if (mIconOnly) {
            qreal border = qMax( 3., qMin(width(), height()) * 0.2);
            if (mMouseOver)
                border = 0.;

            const qreal w = r.width() - 2.*border;
            const qreal h = r.height() - 2.*border;
            qreal borderH;
            qreal borderW;
            const qreal s = qMin(w, h);
            if (h>w) {
                borderH = (r.height() - s)/2.;
                borderW = border;

            } else {
                borderH = border;
                borderW = (r.width() - s)/2.;
            }

            QRectF iconRect(borderW, borderH, s, s);

            QPixmap pixmap = ic.pixmap(iconRect.size().toSize());
            if (!isEnabled())
                painter.setOpacity(0.2);
            painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));

        } else if ( !(ic.isNull()) && !(text().isEmpty())) {
            const int textH = 22;

            qreal border = 5.;
            if (mMouseOver) {
                border = 0.;
            }
            const qreal w = r.width() - 2*border;
            const qreal h = r.height() - border - textH;
            const qreal s = qMin(w, h);

            QRectF iconRect((r.width() - s)/2., border, s, s);
            QPixmap pixmap = ic.pixmap(iconRect.size().toSize());
            if (!isEnabled())
                painter.setOpacity(0.2);

            painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
            painter.setOpacity(1);
            painter.setPen(QColor(200, 200, 200));
            painter.drawText(r.adjusted(0, r.height() - textH, 0, 0), Qt::AlignCenter , text());
        }
    } else {

        r.adjust(1, 1, -1, -1);

        QColor gradColTop(255, 255, 255);
        QColor gradColBot(220, 220, 220);
        QColor penCol(Painting::borderDark);

        if (mColorState == eReady) {
            gradColTop = QColor(91, 196, 98);
            gradColBot = QColor(73, 161, 90);
            penCol = Qt::white;

        } else if (mColorState == eWarning) {
            gradColTop = QColor(255, 151, 123);
            gradColBot = QColor(140, 20, 20);
            penCol = Qt::white;
        }

        if (!isEnabled()) {
            gradColTop = QColor(160, 160, 160);
            gradColBot = QColor(160, 160, 160);

        } else if (isDown() || isChecked()) {
            gradColTop = QColor(200, 200, 200);
            gradColBot = QColor(220, 220, 220);
        }

        QLinearGradient grad(0, 0, 0, r.height());
        grad.setColorAt(0, gradColTop);
        grad.setColorAt(1, gradColBot);
        painter.setBrush(grad);
        painter.setPen(QColor(120, 120, 120));
        const int boxRoundedRadus= int (ceil(font().pointSize() * 0.3));
        painter.drawRoundedRect(r, boxRoundedRadus, boxRoundedRadus);

        painter.setPen(penCol);
        painter.drawText(r, Qt::AlignCenter, text());

    }

    if (mMouseOver && AppSettings::mShowHelp) {
        QToolTip::showText(mapToGlobal(rect().center()), toolTip());
    }

    painter.restore();

}
