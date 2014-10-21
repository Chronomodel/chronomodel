#ifndef MCMCSETTINGSDIALOG_H
#define MCMCSETTINGSDIALOG_H

#include <QDialog>
#include "MCMCSettings.h"

class LineEdit;
class QGroupBox;
class Collapsible;
class Label;
class Button;


class MCMCSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCSettingsDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
    virtual ~MCMCSettingsDialog();

    void setSettings(const MCMCSettings& settings);
    MCMCSettings getSettings();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private slots:
    void adaptSize();
    
private:
    Label* mNumProcLab;
    Label* mNumBurnLab;
    Label* mNumIterLab;
    Label* mMaxBatchesLab;
    Label* mIterPerBatchLab;
    Label* mDownSamplingLab;
    
    LineEdit* mNumProcEdit;
    LineEdit* mNumBurnEdit;
    LineEdit* mNumIterEdit;
    LineEdit* mMaxBatchesEdit;
    LineEdit* mIterPerBatchEdit;
    LineEdit* mDownSamplingEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
    
    Collapsible* mAdvanced;
    QWidget* mAdvancedWidget;
    
    int mWidth;
    int mMargin;
    int mLineH;
    int mButW;
    int mButH;
};

#endif
