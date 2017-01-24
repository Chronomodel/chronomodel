#ifndef MCMCProgressDialog_H
#define MCMCProgressDialog_H

#include <QDialog>

class MCMCLoop;
class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;


class MCMCProgressDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCProgressDialog(MCMCLoop* loop, QWidget* parent = 0, Qt::WindowFlags flags = 0);
    ~MCMCProgressDialog();
    
    int startMCMC();
    
public slots:
    void cancelMCMC();
    //void addMessage(QString message);
    
    void setTitle1(const QString& message, int minProgress, int maxProgress);
    void setProgress1(int value);
    
    void setTitle2(const QString& message, int minProgress, int maxProgress);
    void setProgress2(int value);
    
    void setFinishedState();
    
protected:
    void keyPressEvent(QKeyEvent* e);
    
public:
    MCMCLoop* mLoop;
    QLabel* mLabel1;
    QLabel* mLabel2;
    QProgressBar* mProgressBar1;
    QProgressBar* mProgressBar2;
   // QPushButton* mOKBut;
    QPushButton* mCancelBut;
};

#endif
