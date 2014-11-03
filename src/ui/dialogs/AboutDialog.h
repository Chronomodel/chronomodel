#ifndef AboutDialog_H
#define AboutDialog_H

#include <QDialog>

class QLabel;
class Button;


class AboutDialog: public QDialog
{
    Q_OBJECT
public:
    AboutDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~AboutDialog();
    
private slots:
    void showLicense();
    
public:
    QLabel* mLabel;
    Button* mLicenseBut;
};

#endif
