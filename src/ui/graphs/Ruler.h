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
     void addArea(int start, int end, const QColor& color);
     // Set value formatting functions
     void setFormatFunctX(DateConversion f);
    void updateLayout();
protected:

    void resizeEvent(QResizeEvent* e);
    void paintEvent(QPaintEvent* e);

    DateConversion mFormatFuncX;

public slots:
    void setZoom(double & prop);
    void updateScroll();
    void scrollValueChanged(double value);


signals:
    void positionChanged(double min, double max);
    void valueChanged(double value);

private:
    QScrollBar* mScrollBar;
    qreal mScrollBarHeight;
    QFont mAxisFont;
    QRectF mAxisRect;

    qreal mStepMinWidth;
    qreal mStepWidth;
    qreal mMarginLeft;
    qreal mMarginRight;
    qreal mMarginTop;
    qreal mMarginBottom;

    AxisTool mAxisTool;


    QVector<RulerArea> mAreas;
};

#endif
