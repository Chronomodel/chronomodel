#ifndef MCMCSETTINGSDIALOG_H
#define MCMCSETTINGSDIALOG_H

#include <QDialog>
#include <QLabel>
#include "MCMCSettings.h"

class Label;
class LineEdit;
class Button;
class QDialogButtonBox;
class QSpinBox;
class HelpWidget;


class MCMCSettingsDialog: public QDialog
{
    Q_OBJECT
public:
    MCMCSettingsDialog(QWidget* parent = nullptr);
    virtual ~MCMCSettingsDialog();

    void setSettings(const MCMCSettings& settings);
    MCMCSettings getSettings();

protected slots:
    void inputControl();
    void reset();
    void setQuickTest();

signals:
    void inputValided();
    void inputRejected();

protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent* e);
    void updateLayout();
    
private:
    // Dimension

   int mTop;
   int mColoredBoxHeigth; // heigth of the colored box
   int mBurnBoxWidth;
   int mAdaptBoxWidth;// total width of the colored boxes
   int mAcquireBoxWidth;

   int mBottom;

   int mLineH;
   int mEditW;

   int mButW ;
   int mButH ; // size of the button OK and Cancel
   int mMargin;

   int mTotalWidth;
   // Object

    QFontMetrics *fm;
    QLabel *mNumProcLabel;
    LineEdit* mNumProcEdit;

    QLabel *mTitleBurnLabel;
    QLabel *mIterBurnLabel;
    LineEdit* mNumBurnEdit;

    LineEdit* mNumIterEdit;
    LineEdit* mMaxBatchesEdit;
    LineEdit* mIterPerBatchEdit;
    LineEdit* mDownSamplingEdit;

    QLabel *mSeedsLabel;
    LineEdit* mSeedsEdit;
    HelpWidget* mHelp;

    QLabel* mLevelLabel;
    LineEdit* mLevelEdit;

    Button* mOkBut;
    Button* mCancelBut;

    Button* mTestBut;
    Button* mResetBut;

    QRectF mBurnRect;
    QRectF mAdaptRect;
    QRectF mAquireRect;
    QRectF mBatch1Rect;
    QRectF mBatchInterRect;
    QRectF mBatchNRect;
    

};

#endif
