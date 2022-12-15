/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2018

Authors :
	Philippe LANOS
	Helori LANOS
 	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This software is governed by the CeCILL V2.1 license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL V2.1 license and that you accept its terms.
--------------------------------------------------------------------- */

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
#include "ScrollCompressor.h"

#include "ProjectSettings.h"

class MultiCalibrationView: public QWidget
{
    Q_OBJECT
public:
    MultiCalibrationView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);

    ~MultiCalibrationView();

    void setEventsList(const QList<Event*> &list) {mEventsList = list;}
    void setProject(Project *project) {mProject = project;}

    void updateGraphList();
    void initScale (const double &majorScale, const int &minorScale) { mMajorScale= majorScale; mMinorScale = minorScale;}
    void initScale (const Scale &s) { mMajorScale = s.mark; mMinorScale = s.tip;}


protected:
    void paintEvent(QPaintEvent* e);
    void resizeEvent(QResizeEvent*);
    void updateLayout();
    MultiCalibrationDrawing* scatterPlot(const double thres);
    MultiCalibrationDrawing* multiCalibrationPlot(const double thres);


public slots:
    virtual void setVisible(bool visible);
    void updateMultiCalib(); // come from ModelView
    void updateScaleX();
    void applyAppSettings();

private slots:
    void updateHPDGraphs(const QString & thres);

    void updateGraphsSize(const QString & sizeStr);
    void updateYZoom(const double prop);

    void updateGraphsZoom();
    void updateScroll();
    void exportImage();
    void exportFullImage();
    void copyImage();
    void copyText();
    void exportResults();

    void changeCurveColor();
    void showStat();
    void showScatter();

signals:
    void closed();

public:
    QList<Event*> mEventsList;
    ProjectSettings mSettings;
    Project *mProject;

private:

    MultiCalibrationDrawing* mDrawing;

    QTextEdit* mStatArea;
    int mButtonWidth;
    int mButtonHeigth;

    // member for graphs
    int mMarginLeft;
    int mMarginRight;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mStatClipBut;
    Button* mExportResults;
    Button* mColorClipBut;
    Button* mScatterClipBut;
    ColorPicker* mColorPicker;

    //QFrame* frameSeparator;

    Label* mGraphHeightLab;
    LineEdit* mGraphHeightEdit;
    ScrollCompressor* mYZoom;

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
