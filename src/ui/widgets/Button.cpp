#include "Button.h"
#include "Painting.h"
#include <QtWidgets>


Button::Button(QWidget* parent):QPushButton(parent)
{
    init();
}

Button::Button(const QString& text, QWidget* parent):QPushButton(text, parent)
{
    init();
}

void Button::init()
{
    setCursor(Qt::PointingHandCursor);
    mFlatVertical = false;
    mFlatHorizontal = false;
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

void Button::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    
    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    
    QFont font = painter.font();
    font.setPointSizeF(pointSize(10.f));
    painter.setFont(font);
    
    QRectF r = rect();
    
    if(mFlatVertical || mFlatHorizontal)
    {
        QColor gradColTop(40, 40, 40);
        QColor gradColBot(30, 30, 30);
        QColor gradLineLight(50, 50, 50);
        QColor gradLineDark(0, 0, 0);
        
        if(!isEnabled())
        {
            gradColTop = QColor(110, 110, 110);
            gradColBot = QColor(100, 100, 100);
            gradLineLight = QColor(120, 120, 120);
            gradLineDark = QColor(80, 80, 80);
        }
        else if(isDown() || isChecked())
        {
            //gradColTop = QColor(20, 20, 20);
            //gradColBot = QColor(10, 10, 10);
            gradColTop = mainColorDark;
            gradColBot = mainColorDark;
            gradLineLight = QColor(30, 30, 30);
            gradLineDark = QColor(10, 10, 10);
        }
        QLinearGradient grad(0, 0, 0, r.height());
        grad.setColorAt(0, gradColTop);
        grad.setColorAt(1, gradColBot);
        painter.fillRect(r, grad);
        
        painter.setPen(gradLineLight);
        if(mFlatVertical)
            painter.drawLine(0, 0, r.width(), 0);
        else if(mFlatHorizontal)
            painter.drawLine(0, 0, 0, r.height());
        
        painter.setPen(gradLineDark);
        if(mFlatVertical)
            painter.drawLine(0, r.height(), r.width(), r.height());
        else if(mFlatHorizontal)
            painter.drawLine(r.width(), 0, r.width(), r.height());
        
        // ---------
        
        QIcon ic = icon();
        painter.setPen(QColor(200, 200, 200));
        
        if(!ic.isNull() && !text().isEmpty())
        {
            int textH = 22;
            if(ic.isNull())
                textH = height();
            
            float m = 8;
            float w = r.width() - 2*m;
            float h = r.height() - m - textH;
            float s = qMin(w, h);
            
            painter.drawText(r.adjusted(0, r.height() - textH, 0, 0), Qt::AlignCenter, text());
            
            QRectF iconRect((r.width() - s)/2.f, m, s, s);
            QPixmap pixmap = ic.pixmap(iconRect.size().toSize());
            painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
        }
        else if(!text().isEmpty())
        {
            painter.drawText(r, Qt::AlignCenter, text());
        }
        else if(!ic.isNull())
        {
            float m = 5;
            float w = r.width() - 2*m;
            float h = r.height() - 2*m;
            float s = qMin(w, h);
            
            QRectF iconRect((r.width() - s)/2.f, m, s, s);
            QPixmap pixmap = ic.pixmap(iconRect.size().toSize());
            painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
        }
    }
    else
    {
        r.adjust(1, 1, -1, -1);
        
        QColor gradColTop(255, 255, 255);
        QColor gradColBot(220, 220, 220);
        
        if(!isEnabled())
        {
            gradColTop = QColor(160, 160, 160);
            gradColBot = QColor(160, 160, 160);
        }
        else if(isDown() || isChecked())
        {
            gradColTop = QColor(200, 200, 200);
            gradColBot = QColor(220, 220, 220);
        }
        
        QLinearGradient grad(0, 0, 0, r.height());
        grad.setColorAt(0, gradColTop);
        grad.setColorAt(1, gradColBot);
        painter.setBrush(grad);
        painter.setPen(QColor(120, 120, 120));
        painter.drawRoundedRect(r, 5, 5);
        
        painter.setPen(QColor(50, 50, 50));
        painter.drawText(r, Qt::AlignCenter, text());
    }
    
    painter.restore();
}

