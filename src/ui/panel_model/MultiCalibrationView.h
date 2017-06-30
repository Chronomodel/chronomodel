#ifndef MULTICALIBRATIONVIEW_H
#define MULTICALIBRATIONVIEW_H

#include <QObject>
#include <QWidget>
#include <QFrame>

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
    MultiCalibrationView(QWidget* parent = nullptr, Qt::WindowFlags flags = 0);

    ~MultiCalibrationView();

    void setEventsList(const QList<Event*> &list) {mEventsList = list;}
    void setProject(Project *project) {mProject = project;}
    void setFont(const QFont& font);
    void updateGraphList();


protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent*);
    void updateLayout();

public slots:
    virtual void setVisible(bool visible);
    void updateMultiCalib(); // come from ModelView
    void changeCurveColor();


private slots:
    void updateHPDGraphs(const QString & thres);
    void updateGraphsSize(const QString & size);
    void updateGraphsZoom();
    void updateScroll();
    void exportImage();
    void exportFullImage();
    void copyImage();
    void copyText();



signals:
    void closed();

public:
    QList<Event*> mEventsList;
    ProjectSettings mSettings;
    Project *mProject;

private:

    MultiCalibrationDrawing* mDrawing;

    qreal mButtonWidth;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;
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

    double mTminDisplay;
    double mTmaxDisplay;
    double mThreshold;
    double mGraphHeight;
    QColor mCurveColor;

    QString mResultText;

};

#endif // MULTICALIBRATIONVIEW_H
