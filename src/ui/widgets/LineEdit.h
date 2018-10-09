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
    virtual ~LineEdit();
     void setAdjustText(bool ad = true);

private:
    void adjustFont();

    bool mAdjustText;

protected:
    void resizeEvent(QResizeEvent* e);
};

#endif
