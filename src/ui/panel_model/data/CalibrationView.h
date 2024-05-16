/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2024

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

#ifndef CALIBRATIONVIEW_H
#define CALIBRATIONVIEW_H

#include <QWidget>
#include <QJsonObject>

#include "Date.h"
#include "StudyPeriodSettings.h"
#include "GraphView.h"

class GraphViewRefAbstract;

class Marker;
class LineEdit;
class Button;
class QTextEdit;
class QLabel;
class Label;
class QSlider;
class QScrollBar;
class QGraphicsScene;
class QGraphicsView;
class CalibrationDrawing;


class CalibrationView: public QWidget
{
    Q_OBJECT
public:
    CalibrationView(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::Widget);
    ~CalibrationView();

    void setDate(const QJsonObject& date);
    void setDate(const Date& date);
    void resetDate() {mDate = Date();};
   // void setFont(const QFont& font);
    void initScale (const double majorScale, const int minorScale) { mMajorScale= majorScale; mMinorScale = minorScale;}
    void initScale (const Scale &s) { mMajorScale = s.mark; mMinorScale = s.tip;}

protected:
    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);
    void updateLayout();

public slots:
    virtual void setVisible(bool visible);
    void updateScaleX();
    void updateGraphs();
    void applyAppSettings();

private slots:

    void updateZoom();
    void updateScroll();
    void applyStudyPeriod();
    void exportImage();
    void copyImage();
    void copyText();


signals:
    void closed();

public:
    Date mDate;
    StudyPeriodSettings mSettings;

private:

    CalibrationDrawing* mDrawing;

    GraphView* mCalibGraph;
    GraphViewRefAbstract* mRefGraphView;

    QTextEdit* mResultsText;
   // qreal mResultsHeight;

    int mButtonWidth;
    int mButtonHeigth;

    Button* mImageSaveBut;
    Button* mImageClipBut;
    Button* mResultsClipBut;

    Label* mHPDLab;
    LineEdit* mHPDEdit;

    Label* mStartLab;
    LineEdit* mStartEdit;

    Label* mEndLab;
    LineEdit* mEndEdit;

    // Adjust the zoom on the study period
    QPushButton* mDisplayStudyBut;

    Label* mMajorScaleLab;
    LineEdit* mMajorScaleEdit;

    Label* mMinorScaleLab;
    LineEdit* mMinorScaleEdit;

    double mTminDisplay;
    double mTmaxDisplay;

    double mMajorScale;
    int mMinorScale;
};

#endif
