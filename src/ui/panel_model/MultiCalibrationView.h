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

#ifndef MULTICALIBRATIONVIEW_H
#define MULTICALIBRATIONVIEW_H

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QTextEdit>

#include "Button.h"
#include "Label.h"
#include "LineEdit.h"
#include "MultiCalibrationDrawing.h"
#include "ColorPicker.h"
#include "MultiplotColorDialog.h"
#include "ScrollCompressor.h"
#include "CurveUtilities.h"

#include "StudyPeriodSettings.h"

class MultiCalibrationView: public QWidget
{
    Q_OBJECT
public:
    StudyPeriodSettings mSettings;

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
    Button* mFitClipBut;
    Button* mColorPicker;

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
    qreal mGraphHeight;
    qreal mHeightForVisibleTicksAxis;

    bool mUsePluginColor;
    bool mUseEventColor;
    bool mUseCustomColor;
    QColor mCurveCustomColor;

    QString mResultText;

    SilvermanParam mSilverParam;

public:
    MultiCalibrationView(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::Widget);

    ~MultiCalibrationView();

    void setProject();

    void updateGraphList();
    void initScale (const double majorScale, const int minorScale) { mMajorScale = majorScale; mMinorScale = minorScale;}
    void initScale (const Scale &s) { mMajorScale = s.mark; mMinorScale = s.tip;}


protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent*);
    void updateLayout();
    MultiCalibrationDrawing* scatterPlot(const double thres);
    MultiCalibrationDrawing* fitPlot(const double thres);
    MultiCalibrationDrawing* multiCalibrationPlot(const double thres);


public slots:
    virtual void setVisible(bool visible);
    void updateMultiCalib(); // come from ModelView
    void updateScaleX();
    void applyAppSettings();

private slots:
    void updateHPDGraphs(const QString &thres);

    void updateGraphsSize(const QString &sizeStr);
    void updateYZoom(const double prop = 100);

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
    void showFit();

signals:
    void closed();


};

#endif // MULTICALIBRATIONVIEW_H
