#include "ColorPicker.h"
#include <QtWidgets>


ColorPicker::ColorPicker(const QColor& color, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mColor(color)
{
   
}

ColorPicker::~ColorPicker()
{

}

void ColorPicker::setColor(const QColor& color)
{
    mColor = color;
    update();
}

QColor ColorPicker::getColor() const
{
    return mColor;
}

void ColorPicker::openDialog()
{
    QColor color = QColorDialog::getColor(mColor, qApp->activeWindow(), tr("Select Phase Color"));
    if(color.isValid())
    {
        setColor(color);
        emit colorChanged(color);
    }
}

void ColorPicker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QRectF roundedRect(rect().x() + borderSize / 2,
                rect().y() + borderSize / 2,
                rect().width() - borderSize,
                rect().height() - borderSize);
    
    pen.setWidth(borderSize);
    pen.setColor(isEnabled() ? mColor : QColor(220, 220, 220));
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(pen);
    
    painter.drawRoundedRect(roundedRect.translated(0.5, 0.5), borderRadius, borderRadius);
    
    painter.fillRect(roundedRect, pen.color());
    
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    if(isEnabled())
        openDialog();
    Q_UNUSED(e);
}
