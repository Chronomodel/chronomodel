#ifndef Button_H
#define Button_H

#include <QPushButton>


class Button: public QPushButton
{
    Q_OBJECT
public:
    explicit Button(QWidget* parent = 0);
    explicit Button(const QString& text, QWidget* parent = 0);
    
    //Button(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~Button();
    
protected:
    void paintEvent(QPaintEvent* e);
};

#endif
