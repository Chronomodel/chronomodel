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

#ifndef RULER_H
#define RULER_H

#include <QWidget>
#include "AxisTool.h"


class QScrollBar;

struct RulerArea{
    int mStart;
    int mStop;
    QColor mColor;
};

class Ruler: public QWidget
{
    Q_OBJECT
public:
    Ruler(QWidget* parent = nullptr, Qt::WindowFlags flags =  Qt::Widget);
    ~Ruler();
    Ruler& operator=(const Ruler & origin);

    double realPosition;
    double mCurrentMin;
    double mCurrentMax;
    double mMin;
    double mMax;
    double mZoomProp;
    void setFont(const QFont &font);

    void setRange(const double min, const double max);
    void setCurrent(const double min, const double max);
    void setMarginTop (const qreal &top) {mMarginTop = top;}
    void setMarginBottom (const qreal &bottom) {mMarginBottom = bottom;}
    void setMarginRight (const qreal &right) {mMarginRight = right;}
    void setMarginLeft (const qreal &left) {mMarginLeft = left;}

    void currentChanged(const double &min, const double &max);
    void setScaleDivision (const double &major, const int &minorCount);
    void setScaleDivision (const Scale &sc);

    double getZoom();
    double getRealValue();

    void clearAreas();
    void addArea(int start, int end, const QColor &color);
    // Set value formatting functions
    void setFormatFunctX(DateConversion f);
    void updateLayout();
protected:

    bool event(QEvent* event);
    void resizeEvent(QResizeEvent*);
    void paintEvent(QPaintEvent* e);

    DateConversion mFormatFuncX;

public slots:
    void setZoom(double& prop);
    void updateScroll();
    //void scrollValueChanged(double value);
    void wheelScene(QEvent* e) {this->event(e); QWidget::event(e);}

signals:
    void positionChanged(double min, double max);
    //void valueChanged(double value);

private:
    QScrollBar* mScrollBar;
    AxisTool mAxisTool;
    QList<RulerArea> mAreas;
    
    QFont mAxisFont;
    QRectF mAxisRect;

    qreal mStepMinWidth;
    qreal mStepWidth;
    
    qreal mMarginLeft;
    qreal mMarginRight;
    qreal mMarginTop;
    qreal mMarginBottom;
    
public:
    static int sHeight;
};

#endif
