#include "Button.h"
#include "Painting.h"
#include <QtWidgets>


Button::Button(QWidget* parent):QPushButton(parent)
{
    init();

}

Button::Button(const QString& text, QWidget* parent):QPushButton(parent)
{
    setText(text);
    init();
}

void Button::init()
{
    setAutoRepeat(false);
    setCursor(Qt::PointingHandCursor);

    mMouseOver = false;
    mFlatVertical = false;
    mFlatHorizontal = false;
    mIsClose = false;
    
    mColorState = eDefault;
    
    mUseMargin = false;
    mIconOnly = true;
}

Button::~Button()
{

}

void Button::setFlatVertical()
{
    mFlatVertical = true;
    update();
}

void Button::setFlatHorizontal()
{
    mFlatHorizontal = true;
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

void Button::enterEvent(QEvent *e)
{
    mMouseOver = true;
    update();
    if (QPushButton::isCheckable())
        QPushButton::QWidget::enterEvent(e);
}
void Button::leaveEvent(QEvent * e)
{
    mMouseOver = false;
    update();
    QPushButton::QWidget::leaveEvent(e);
}

void Button::isCheckable(const bool checkable)
{
    if (!checkable)
        mColorState = eWarning;
    else
        mColorState = eDefault;

    QPushButton::setCheckable(checkable);

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

        QColor gradColTop(40, 40, 40);
        QColor gradColBot(30, 30, 30);
        QColor gradLineLight(50, 50, 50);
        QColor gradLineDark(0, 0, 0);
        
     /*   if (mMouseOver) {
            gradColTop = QColor(160, 160, 160);
            gradColBot = QColor(50, 50, 50);
            gradLineLight = QColor(70, 70, 70);
            gradLineDark = QColor(20, 20, 20);
        }
*/
        if (!isEnabled()) {
            gradColTop = QColor(110, 110, 110);
            gradColBot = QColor(100, 100, 100);
            gradLineLight = QColor(120, 120, 120);
            gradLineDark = QColor(80, 80, 80);

        } else if (isDown() || isChecked()) {
            gradColTop = Painting::mainColorDark;
            gradColBot = Painting::mainColorDark;
            gradLineLight = QColor(30, 30, 30);
            gradLineDark = QColor(10, 10, 10);
        }
        QLinearGradient grad(0, 0, 0, r.height());
        grad.setColorAt(0, gradColTop);
        grad.setColorAt(1, gradColBot);

        if (mMouseOver)
            painter.fillRect(r.adjusted(-50, -50, 50, 50), grad);
        else
            painter.fillRect(r, grad);
        
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
        
        // ---------

        QIcon ic = icon();
        painter.setPen(QColor(200, 200, 200));
        
        bool iconOnly = mIconOnly;
        bool textOnly = !mIconOnly && !text().isEmpty() && ic.isNull();

        QFont adaptedFont (font());
        QFontMetricsF fm (adaptedFont);
        qreal textSize = fm.width(text());
        if (textSize > (r.width() - 10. )) {
            const qreal fontRate = textSize / (r.width() - 10. );
            const qreal ptSiz = adaptedFont.pointSizeF() / fontRate;
            adaptedFont.setPointSizeF(ptSiz);
        }
        painter.setFont(adaptedFont);

        if (textOnly) {

            if (mMouseOver) {
                 painter.setPen(QColor(250, 250, 250));
                 painter.drawText(r.adjusted(-50, -50, 50, 50), Qt::AlignHCenter | Qt::AlignVCenter, text());

            } else {
                painter.setPen(QColor(200, 200, 200));
                painter.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, text());
            }

        } else if (iconOnly) {
            qreal border = qMax( 3., qMin(width(), height()) * 0.2);
            if (mMouseOver)
                border = 0.;

            const qreal w = r.width() - 2.*border;
            const qreal h = r.height() - 2.*border;
            qreal borderH(0.);
            qreal borderW(0.);
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
            int textH = 22;
            
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
        QColor penCol(50, 50, 50);
        
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
        painter.drawRoundedRect(r, 5, 5);
        
        // TODO : color state
        
        painter.setPen(penCol);
        painter.drawText(r, Qt::AlignCenter, text());

    }
    
    painter.restore();

}

