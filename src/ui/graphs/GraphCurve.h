/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2023

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

#ifndef GRAPHCURVE_H
#define GRAPHCURVE_H

#include "Matrix.h"
#include "Painting.h"

#include <QMap>
#include <QString>
#include <QPen>

typedef double type_data;

class CurveRefPts
{
public:
    enum PointType
    {
        eCross = 'C',
        ePoint = 'P',
        eLine = 'L',
        eDotLine = 'D'
    };

    PointType type;
    double Xmin, Xmax;
    double Ymin, Ymax;
    QColor color;
    QPen pen;
    QBrush brush;
    QString name;

protected:
    bool mVisible;

public:
    explicit CurveRefPts();
    virtual ~CurveRefPts();

    inline bool isVisible() const {return mVisible;};
    inline void setVisible(const bool visible) {mVisible = visible;};

};

class GraphCurve
{
public:
    explicit GraphCurve();
    virtual ~GraphCurve();

    void setPen(QPen pen);

    enum CurveType
    {
        eHisto,
        eDensityData,
        eQVectorData,
        eHorizontalLine,
        eVerticalLine,
        eHorizontalSections,
        eTopLineSections,
        eVerticalQMap,
        eRefPoints,
        eMapData,
        eShapeData,
        eFunctionData

    };

//private:
    CurveType mType;
    QMap<type_data, type_data> mData;
    CurveMap mMap;

    QString mName;
    QPen mPen;
    QBrush mBrush;

    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Unif, Typo!

    QVector<type_data> mDataVector;
    type_data mHorizontalValue;
    type_data mVerticalValue;

    std::vector<QPair<type_data, type_data> > mSections;

    //std::vector<CurveRefPts> mRefPoints;

    std::pair<QMap<type_data, type_data>, QMap<type_data, type_data>> mShape;

    bool mVisible;

public :
    inline bool isHisto() const {return mType == eHisto;}
    inline bool isVectorData() const {return mType == eQVectorData;}

    inline bool isHorizontalLine() const {return mType == eHorizontalLine;}
    inline bool isVerticalLine() const {return mType == eVerticalLine;}
    inline bool isHorizontalSections() const {return mType == eHorizontalSections;}
    inline bool isTopLineSections() const {return mType == eTopLineSections;}
    inline bool isVertical() const {return mType == eVerticalQMap;}

    inline bool isDensityCurve() const {return mType == eDensityData;}
    inline bool isRefPoints() const {return mType == eRefPoints;}
    inline bool isCurveMap() const {return mType == eMapData;}
    inline bool isShape() const {return mType == eShapeData;}
    inline bool isFunction() const {return mType == eFunctionData;}

    void setBrush(const QBrush& brush)  { mBrush = brush;}
    void setPenStyle(const Qt::PenStyle& penStyle)  { mPen.setStyle(penStyle);}
    void setLineColor(const QColor& lineColor)  { mPen.setColor(lineColor);}
};

GraphCurve densityCurve(const QMap<double, double> data,
                                const QString& name,
                                const QColor& lineColor,
                                const Qt::PenStyle penStyle = Qt::SolidLine,
                                const QBrush& brush = Qt::NoBrush) ;

GraphCurve FunctionCurve(const QMap<double, double> data,
                                const QString& name,
                                const QColor& lineColor,
                                const Qt::PenStyle penStyle = Qt::SolidLine,
                                const QBrush& brush = Qt::NoBrush) ;

GraphCurve HPDCurve (QMap<double, double> data,
                     const QString& name, const QColor& color) ;

GraphCurve topLineSection (const std::pair<double, double>& section,
                           const QString& name,
                           const QColor& color) ;

GraphCurve horizontalSection (const std::pair<double, double>& section,
                              const QString& name,
                              const QColor& color = Painting::mainColorLight,
                              const QBrush& brush = Qt::NoBrush) ;

GraphCurve horizontalLine (const double yValue,
                           const QString& name,
                           const QColor& color,
                           const Qt::PenStyle penStyle = Qt::SolidLine) ;

GraphCurve shapeCurve (const QMap<double, double>& dataInf, const QMap<double, double>& dataSup,
                       const QString& name, const QColor& lineColor, const Qt::PenStyle penStyle,
                       const QBrush& brush) ;



#endif
