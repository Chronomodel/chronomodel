#ifndef Button_H
#define Button_H

#include <QPushButton>


class Button: public QPushButton
{
    Q_OBJECT
public:
    Button(QWidget* parent = 0);
    Button(const QString& text, QWidget* parent = 0);
    ~Button();
    void init();
    
    void setFlatVertical();
    void setFlatHorizontal();
    
protected:
    void paintEvent(QPaintEvent* e);
    
    bool mFlatVertical;
    bool mFlatHorizontal;
};

#endif
