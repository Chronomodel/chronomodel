#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QWidget>
#include <QPen>


class ColorPicker: public QWidget
{
    Q_OBJECT
public:
    ColorPicker(const QColor& color = Qt::red, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~ColorPicker();
    
    void setColor(const QColor& color);
   
    QColor getColor() const;
    void openDialog();
    
signals:
    void colorChanged(QColor c);
    
protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);
    
private:
    QColor mColor;
};

#endif
