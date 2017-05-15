#include "Painting.h"
#include "QtUtilities.h"


QColor Painting::mainColorLight = QColor(49, 112, 176);
QColor Painting::mainColorDark = QColor(31, 80, 128);
QColor Painting::mainColorGrey = QColor(187, 187, 187);
QList<QColor> Painting::chainColors = QList<QColor>();
QColor Painting::greyedOut = QColor(255, 255, 255, 200);
QColor Painting::mainGreen = QColor(0, 169, 157);

void Painting::init()
{
    chainColors.append(Qt::blue);
    chainColors.append(Qt::green);
    chainColors.append(Qt::red);
    chainColors.append(Qt::yellow);
    
    for (int i=0; i<200; ++i)
        chainColors.append(randomColor());
}


double pointSize(double size)
{
#if defined(QT_OS_MAC)
    
    return size;
    
#elif defined(QT_OS_WIN32) || defined(WIN32)
    
    return size * 72. / 96.;
    
#endif
    
    return size;
}

void drawButton(QPainter& painter, const QRectF& rect, bool hover, bool isEnabled, const QString& text, const QIcon& icon)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    
    if ( isEnabled && hover) {
        QLinearGradient grad(0, 0, 0, rect.height());
        grad.setColorAt(0, QColor(0, 0, 0));
        grad.setColorAt(1, QColor(20, 20, 20));
        painter.fillRect(rect, grad);
        
        painter.setPen(QColor(10, 10, 10));
        painter.drawLine(0, 0, rect.width(), 0);
        
        painter.setPen(QColor(30, 30, 30));
        painter.drawLine(0, rect.height(), rect.width(), rect.height());
    } else {
        QLinearGradient grad(0, 0, 0, rect.height());
        grad.setColorAt(0, QColor(40, 40, 40));
        grad.setColorAt(1, QColor(30, 30, 30));
        painter.fillRect(rect, grad);
        
        painter.setPen(QColor(50, 50, 50));
        painter.drawLine(0, 0, rect.width(), 0);
        
        painter.setPen(Qt::black);
        painter.drawLine(0, rect.height(), rect.width(), rect.height());
    }
    
    int textH (22);
    
    QFont font = painter.font();
    painter.setFont(font);
    
    painter.setPen(QColor(200, 200, 200));
    painter.drawText(rect.adjusted(0, rect.height() - textH, 0, 0), Qt::AlignCenter, text);
    
    double m = 8.;
    double w = rect.width() - 2.*m;
    double h = rect.height() - m - textH;
    double s = qMin(w, h);
    
    QRectF iconRect((rect.width() - s)/2.f, m, s, s);
    QPixmap pixmap = icon.pixmap(iconRect.size().toSize());
    painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
    
    painter.restore();
}

void drawButton2(QPainter& painter, const QRectF& rect, bool hover, bool isEnabled, const QString& text, const QIcon& icon, bool isFlat)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect;
    
    if (isFlat) {
        painter.setPen(hover ? Qt::black : QColor(50, 50, 50));
        painter.setBrush(hover ? Qt::black : QColor(50, 50, 50));
        painter.drawRect(r);
    } else {
        r = rect.adjusted(1, 1, -1, -1);
        
        QLinearGradient grad(r.x(), r.y(), r.x(), r.y() + r.height());
        if (hover) {
            grad.setColorAt(0, QColor(48, 116, 159));
            grad.setColorAt(1, QColor(22, 70, 103));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 4, 4);
            
            grad.setColorAt(0, QColor(50, 82, 101));
            grad.setColorAt(1, QColor(40, 68, 82));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r.adjusted(2, 2, -2, -2), 2, 2);
            
            painter.setPen(QColor(27, 51, 59));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(r, 4, 4);
        } else if(!isEnabled) {
            painter.setPen(QColor(140, 140, 140));
            painter.setBrush(QColor(160, 160, 160));
            painter.drawRoundedRect(r, 4, 4);
        } else {
            grad.setColorAt(0, QColor(90, 90, 90));
            grad.setColorAt(1, QColor(70, 70, 70));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 4, 4);
            painter.setPen(QColor(110, 110, 110));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(r.adjusted(0, 1, 0, 0), 4, 4);
            painter.setPen(QColor(50, 50, 50));
            painter.drawRoundedRect(r, 4, 4);
        }
    }
    
    QFont font = painter.font();
    painter.setFont(font);
    
    if (!icon.isNull()) {
        int s = (r.width() > r.height()) ? r.height() : r.width();
        int sm = 4;
        int gap = 5;
        
        if (!text.isEmpty()) {
            QFontMetrics metrics(painter.font());
            int mh = metrics.height();
            int h = r.height() - gap - mh;
            s = (r.width() > h) ? h : r.width();
            
            painter.setPen(hover ? Qt::white : QColor(200, 200, 200));
            painter.drawText(sm, r.height() - mh - sm, r.width() - 2*sm, mh, Qt::AlignCenter, text);
        }
        s -= 2*sm;
        QRect iconRect(r.x() + (r.width() - s)/2, r.y() + sm, s, s);
        painter.drawPixmap(iconRect, icon.pixmap(200, 200));
    }
    else if (!text.isEmpty()) {
        painter.setPen(hover ? Qt::white : QColor(200, 200, 200));
        painter.drawText(r, Qt::AlignCenter, text);
    }
    
    painter.restore();
}

void drawBox(QPainter& painter, const QRectF& r, const QString& text)
{
    painter.setPen(QColor(50, 50, 50));
    painter.setBrush(QColor(75, 75, 75));
    painter.drawRect(r);
    painter.setBrush(QColor(50, 50, 50));
    painter.drawRect(r.adjusted(0, 0, 0, -r.height() + 20));
    
    QFont font = painter.font();
    painter.setFont(font);
    
    painter.setPen(Qt::white);
    painter.drawText(r.adjusted(5, 0, -5, -r.height() + 20), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawRadio(QPainter& painter, const QRectF& rect, const QString& text, bool toggled)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect.adjusted(1, 1, -1, -1);
    int subM (0);
    
    painter.setPen(QColor(120, 120, 120));
    painter.setBrush(QColor(230, 230, 230));
    painter.drawEllipse(r.adjusted(0, subM, r.height() - r.width() - 2*subM, -subM));
    
    if (toggled) {
        int insideM = 3;
        painter.setPen(Qt::NoPen);
        painter.setBrush(Painting::mainColorLight);
        painter.drawEllipse(r.adjusted(insideM,
                                       subM+insideM,
                                       r.height() - r.width() - 2*subM - insideM,
                                       -subM - insideM));
    }
    
    QFont font = painter.font();
    painter.setFont(font);
    
    painter.setPen(Qt::black);
    painter.drawText(r.adjusted(r.height() - 2*subM + 5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawCheckbox(QPainter& painter, const QRectF& r, const QString& text, Qt::CheckState state)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    int subM (0);

    QRectF boxRect = r.adjusted(0, subM, r.height() - r.width() - 2*subM, -subM);
    drawCheckBoxBox(painter, boxRect, state, QColor(230, 230, 230), QColor(120, 120, 120));
    
    QFont font = painter.font();
    painter.setFont(font);
    
    painter.setPen(Qt::black);

    painter.drawText(r.adjusted(r.height() - 2*subM + 5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawCheckBoxBox(QPainter& painter, const QRectF& rect, Qt::CheckState state, const QColor& back, const QColor& border)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRectF r = rect.adjusted(1, 1, -1, -1);
    
    painter.setPen(border);
    painter.setBrush(back);
    painter.drawRect(r);
    
    QPen pen = painter.pen();
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(Painting::mainColorLight);
    painter.setPen(pen);
    
    int mi = 2;
    QRectF lr = r.adjusted(mi, mi, -mi, -mi);
    
    if (state == Qt::Checked) {
        painter.drawLine(lr.x(), lr.y(), lr.x() + lr.width(), lr.y() + lr.height());
        painter.drawLine(lr.x() + lr.width(), lr.y(), lr.x(), lr.y() + lr.height());
    }
    else if (state == Qt::PartiallyChecked)
        painter.drawLine(lr.x(), lr.y() + lr.height()/2, lr.x() + lr.width(), lr.y() + lr.height()/2);

}




