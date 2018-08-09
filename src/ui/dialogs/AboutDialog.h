#ifndef AboutDialog_H
#define AboutDialog_H

#include <QDialog>

class QLabel;


class AboutDialog: public QDialog
{
    Q_OBJECT
public:
    AboutDialog(QWidget* parent = nullptr, Qt::WindowFlags flags =  Qt::Widget);
    ~AboutDialog();
  
protected:
    void paintEvent(QPaintEvent* e);
    
public:
    QLabel* mLabel;
};

#endif
