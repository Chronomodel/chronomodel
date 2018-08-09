#ifndef MCMCProgressDialog_H
#define MCMCProgressDialog_H

#include <QDialog>

class MCMCLoopMain;
class QLabel;
class QProgressBar;
class QTextEdit;
class QPushButton;


class MCMCProgressDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCProgressDialog(MCMCLoopMain* loop, QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Window);
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
    MCMCLoopMain* mLoop;
    QLabel* mLabel1;
    QLabel* mLabel2;
    QProgressBar* mProgressBar1;
    QProgressBar* mProgressBar2;
   // QPushButton* mOKBut;
    QPushButton* mCancelBut;
};

#endif
