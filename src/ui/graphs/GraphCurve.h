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
        eDotLine = 'D',
        eDotLineCross = 'T',
        eCustomDashLineCross ='U',
        eRoundLine = 'R' //For Bound
    };

    PointType type;
    double Xmin, Xmax;
    double Ymin, Ymax;
    QColor color;
    QPen pen;
    QBrush brush;
    QString name;
    QString comment;

protected:
    bool mVisible;

public:
    explicit CurveRefPts();
    virtual ~CurveRefPts();

    inline bool isVisible() const
    {
        return mVisible;
    };
    inline void setVisible(const bool visible)
    {
        mVisible = visible;
    };

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
        eVectorData,
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

    CurveType mType;
    QMap<type_data, type_data> mData;
    CurveMap mMap;

    QString mName;
    QPen mPen;
    QBrush mBrush;
    std::vector<ColorStop> mColorStops;

    bool mIsRectFromZero; // draw a vertical line when graph value leaves 0 : usefull for HPD and Unif, Typo!

    std::vector<type_data> mDataVector;
    type_data mHorizontalValue;
    type_data mVerticalValue;

    std::vector<QPair<type_data, type_data> > mSections;

    std::pair<QMap<type_data, type_data>, QMap<type_data, type_data>> mShape;

    bool mVisible;

public :
    inline bool isHisto() const
    {
        return mType == eHisto;
    }
    inline bool isVectorData() const
    {
        return mType == eVectorData;
    }

    inline bool isHorizontalLine() const
    {
        return mType == eHorizontalLine;
    }
    inline bool isVerticalLine() const
    {
        return mType == eVerticalLine;
    }
    inline bool isHorizontalSections() const
    {
        return mType == eHorizontalSections && !mSections.empty();
    }
    inline bool isTopLineSections() const
    {
        return mType == eTopLineSections  && !mSections.empty();
    }
    inline bool isVertical() const
    {
        return mType == eVerticalQMap;
    }

    inline bool isDensityCurve() const {return mType == eDensityData;}
    inline bool isRefPoints() const {return mType == eRefPoints;}
    inline bool isCurveMap() const {return mType == eMapData;}
    inline bool isShape() const {return mType == eShapeData;}
    inline bool isFunction() const {return mType == eFunctionData;}

    inline void setBrush(const QBrush& brush)
    {
        mBrush = brush;
    }
    inline void setPenStyle(const Qt::PenStyle& penStyle)
    {
        mPen.setStyle(penStyle);
    }
    inline void setLineColor(const QColor& lineColor)
    {
        mPen.setColor(lineColor);
    }

    inline void setPalette(const ColorPalette& palette)
    {
        mColorStops = ColorStops::getStops(palette);
    }
    inline void setColorStops(const std::vector<ColorStop>& stops)
    {
        mColorStops = stops;
    }
};

GraphCurve densityCurve(const QMap<double, double> data,
                                const QString &name,
                                const QColor &lineColor,
                                const Qt::PenStyle penStyle = Qt::SolidLine,
                                const QBrush& brush = Qt::NoBrush) ;

GraphCurve densityCurve(const std::map<double, double> data,
                        const QString &name,
                        const QColor &lineColor,
                        const Qt::PenStyle penStyle = Qt::SolidLine,
                        const QBrush& brush = Qt::NoBrush) ;

GraphCurve FunctionCurve(const QMap<double, double> data,
                                const QString &name,
                                const QColor &lineColor,
                                const Qt::PenStyle penStyle = Qt::SolidLine,
                                const QBrush& brush = Qt::NoBrush,
                                const bool is_visible = true) ;

GraphCurve HPDCurve (QMap<double, double> data,
                     const QString &name, const QColor &color, const bool is_visible = true) ;
GraphCurve HPDCurve (std::map<double, double> data,
                    const QString &name, const QColor &color, const bool is_visible = true) ;

GraphCurve topLineSection (const std::pair<double, double> &section,
                           const QString &name,
                           const QColor &color) ;

GraphCurve horizontalSection (const std::pair<double, double>& section,
                              const QString &name,
                              const QColor &color = Painting::mainColorLight,
                              const QBrush &brush = Qt::NoBrush) ;

GraphCurve horizontalLine (const double yValue,
                           const QString &name,
                           const QColor &color,
                           const Qt::PenStyle penStyle = Qt::SolidLine) ;

GraphCurve shapeCurve (const QMap<double, double> &dataInf, const QMap<double, double> &dataSup,
                       const QString &name, const QColor &lineColor, const Qt::PenStyle penStyle,
                       const QBrush &brush, const bool is_visible = true) ;

GraphCurve shapeCurve (const std::map<double, double> &dataInf, const std::map<double, double> &dataSup,
                      const QString &name, const QColor &lineColor, const Qt::PenStyle penStyle,
                      const QBrush &brush, const bool is_visible = true) ;

#endif
