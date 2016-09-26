#ifndef RadioButton_H
#define RadioButton_H

#include <QRadioButton>


class RadioButton: public QRadioButton
{
    Q_OBJECT
public:
    explicit RadioButton(QWidget* parent = 0);
    explicit RadioButton(const QString& text, QWidget* parent = 0);
    
    ~RadioButton();
    
protected:
    void paintEvent(QPaintEvent* e);
};

#endif
