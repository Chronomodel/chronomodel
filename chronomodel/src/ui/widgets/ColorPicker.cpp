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
    QPainter p(this);
    p.fillRect(rect(), isEnabled() ? mColor : QColor(220, 220, 220));
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    if(isEnabled())
        openDialog();
    Q_UNUSED(e);
}
