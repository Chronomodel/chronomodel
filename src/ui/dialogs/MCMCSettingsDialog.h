#ifndef MCMCSETTINGSDIALOG_H
#define MCMCSETTINGSDIALOG_H

#include <QDialog>
#include "MCMCSettings.h"

class LineEdit;
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
    
private:
    LineEdit* mNumProcEdit;
    LineEdit* mNumBurnEdit;
    LineEdit* mNumIterEdit;
    LineEdit* mMaxBatchesEdit;
    LineEdit* mIterPerBatchEdit;
    LineEdit* mDownSamplingEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
    
    QRectF mBurnRect;
    QRectF mAdaptRect;
    QRectF mAquireRect;
    QRectF mBatch1Rect;
    QRectF mBatchInterRect;
    QRectF mBatchNRect;
};

#endif
