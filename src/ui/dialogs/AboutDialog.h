#ifndef AboutDialog_H
#define AboutDialog_H

#include <QDialog>

class QLabel;


class AboutDialog: public QDialog
{
    Q_OBJECT
public:
    AboutDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~AboutDialog();
    
public:
    QLabel* mLabel;
};

#endif
