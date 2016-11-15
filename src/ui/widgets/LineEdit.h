#ifndef LineEdit_H
#define LineEdit_H

#include <QLineEdit>


class LineEdit: public QLineEdit
{
    Q_OBJECT
public:
    explicit LineEdit(QWidget* parent = 0);
    ~LineEdit();
};

#endif
