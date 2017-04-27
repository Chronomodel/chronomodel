#ifndef CheckBox_H
#define CheckBox_H

#include <QCheckBox>


class CheckBox: public QCheckBox
{
    Q_OBJECT
public:
    explicit CheckBox(QWidget* parent = nullptr);
    explicit CheckBox(const QString& text, QWidget* parent = nullptr);
    
    ~CheckBox();
    
protected:
    void paintEvent(QPaintEvent* e);
};

#endif
