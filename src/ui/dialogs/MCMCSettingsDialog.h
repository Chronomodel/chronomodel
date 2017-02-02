#ifndef MCMCSETTINGSDIALOG_H
#define MCMCSETTINGSDIALOG_H

#include <QDialog>
#include "MCMCSettings.h"

class Label;
class LineEdit;
class Button;
class QSpinBox;
class HelpWidget;


class MCMCSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCSettingsDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);
    virtual ~MCMCSettingsDialog();

    void setSettings(const MCMCSettings& settings);
    MCMCSettings getSettings();

protected slots:
    void inputControl();

signals:
    void inputValided();
    void inputRejected();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private:
    Label* mSeedsLab;
    
    LineEdit* mNumProcEdit;
    LineEdit* mNumBurnEdit;
    LineEdit* mNumIterEdit;
    LineEdit* mMaxBatchesEdit;
    QSpinBox* mIterPerBatchSpin;
    LineEdit* mDownSamplingEdit;
    LineEdit* mSeedsEdit;
    HelpWidget* mHelp;
    
    Label* mLabelLevel;
    LineEdit* mLevelEdit;
    
    Button* mOkBut;
    Button* mCancelBut;
    
    QRectF mBurnRect;
    QRectF mAdaptRect;
    QRectF mAquireRect;
    QRectF mBatch1Rect;
    QRectF mBatchInterRect;
    QRectF mBatchNRect;
     // dimension
    qreal mTotalWidth;
    int mMargin;
    qreal top;
    qreal lineH;
    qreal editW;
    qreal w ;
    qreal h; // size of the colored box
    int butW ;
    int butH ; // size of the button OK and Cancel
};

#endif
