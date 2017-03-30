#ifndef LineEdit_H
#define LineEdit_H

#include <QLineEdit>

class QFont;

class LineEdit: public QLineEdit
{
    Q_OBJECT

public slots:
    virtual void setVisible(bool visible);

public:
    explicit LineEdit(QWidget* parent = nullptr);
    void setFont(const QFont& font);
    ~LineEdit();
};

#endif
