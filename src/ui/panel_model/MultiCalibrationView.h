#ifndef MULTICALIBRATIONVIEW_H
#define MULTICALIBRATIONVIEW_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QTextEdit>

#include "Button.h"
#include "Event.h"
#include "Label.h"
#include "LineEdit.h"
#include "MultiCalibrationDrawing.h"
#include "ColorPicker.h"

#include "ProjectSettings.h"

class MultiCalibrationView: public QWidget
{
    Q_OBJECT
public:
    MultiCalibrationView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);

    ~MultiCalibrationView();

    void setEventsList(const QList<Event*> &list) {mEventsList = list;}
    void setProject(Project *project) {mProject = project;}
  //  void setFont(const QFont& font);
    void updateGraphList();
    void initScale (const double &majorScale, const int &minorScale) { mMajorScale= majorScale; mMinorScale = minorScale;}
    void initScale (const Scale &s) { mMajorScale = s.mark; mMinorScale = s.tip;}


protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent*);
    void updateLayout();

public slots:
    virtual void setVisible(bool visible);
    void updateMultiCalib(); // come from ModelView
    void updateScaleX();
    void applyAppSettings();

private slots:
    void updateHPDGraphs(const QString & thres);
    void updateGraphsSize(const QString & sizeStr);
    void updateGraphsZoom();
    void updateScroll();
    void exportImage();
    void exportFullImage();
    void copyImage();
    void copyText();

    void changeCurveColor();
    void showStat();

signals:
    void closed();

public:
    QList<Event*> mEventsList;
    ProjectSettings mSettings;
    Project *mProject;

private:

    MultiCalibrationDrawing* mDrawing;
    QTextEdit* mTextArea;
    int mButtonWidth;
    int mButtonHeigth;

    // member for graphs
    int mMarginLeft;
    int mMarginRight;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mStatClipBut;
    Button* mColorClipBut;
    ColorPicker* mColorPicker;

    QFrame* frameSeparator;

    Label* mGraphHeightLab;
    LineEdit* mGraphHeightEdit;

    Label* mHPDLab;
    LineEdit* mHPDEdit;

    Label* mStartLab;
    LineEdit* mStartEdit;

    Label* mEndLab;
    LineEdit* mEndEdit;

    Label* mMajorScaleLab;
    LineEdit* mMajorScaleEdit;

    Label* mMinorScaleLab;
    LineEdit* mMinorScaleEdit;

    double mMajorScale;
    int mMinorScale;

    double mTminDisplay;
    double mTmaxDisplay;
    double mThreshold;
    int mGraphHeight;
    QColor mCurveColor;

    QString mResultText;

};

#endif // MULTICALIBRATIONVIEW_H
