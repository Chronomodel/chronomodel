#include "ColorPicker.h"
#include <QtWidgets>


ColorPicker::ColorPicker(const QColor& color, QWidget* parent, Qt::WindowFlags flags):QWidget(parent, flags),
mColor(color)
{
    setMinimumHeight(20);
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
    QColor color = QColorDialog::getColor(mColor, qApp->activeWindow(), tr("Select Color"));
    if (color.isValid()) {
        setColor(color);
        emit colorChanged(color);
    }
}

void ColorPicker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 4, 4);
    painter.fillPath(path, isEnabled() ? mColor : QColor(220, 220, 220));
    painter.strokePath(path, QColor(160, 160, 160));
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if (isEnabled())
        openDialog();

}
