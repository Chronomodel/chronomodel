/* ---------------------------------------------------------------------

Copyright © CNRS	2014 - 2026

Authors :
	Philippe LANOS
	Helori LANOS
	Philippe DUFRESNE

This software is a computer program whose purpose is to
create chronological models of archeological data using Bayesian statistics.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

SPDX-License-Identifier: GPL-3.0-or-later
--------------------------------------------------------------------- */

#ifndef CALIBRATIONVIEW_H
#define CALIBRATIONVIEW_H

#include "Date.h"
#include "StudyPeriodSettings.h"
#include "GraphView.h"

#include <QWidget>
#include <QJsonObject>
#include <QPushButton>

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
