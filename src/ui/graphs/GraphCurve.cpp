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

#include "GraphCurve.h"


CurveRefPts::CurveRefPts():
    type(CurveRefPts::eCross),
    pen(Qt::black, 1),
    brush(Qt::NoBrush)
{
}

CurveRefPts::~CurveRefPts()
{
}

GraphCurve::GraphCurve():
    mType(eDensityData),
    mData (QMap<type_data, type_data>()),
    mName(QString()),
    mPen(Qt::black, 1),
    mBrush(Qt::NoBrush),
    mIsRectFromZero(false),
    mDataVector(std::vector<type_data>()),
    mHorizontalValue(type_data(0)),
    mVerticalValue(type_data(0)),
    mSections(std::vector<QPair<type_data, type_data> >()),
    mShape(std::pair<QMap<type_data, type_data>, QMap<type_data, type_data>> ()),
    mVisible(true)
{
}

GraphCurve::~GraphCurve()
{
    mData.clear();
    mDataVector.clear();
    mSections.clear();

    mShape.first.clear();
    mShape.second.clear();
}


void GraphCurve::setPen(QPen pen)
{
    mPen = pen;
}


/** Generate Typical curves for Chronomodel
 * */
GraphCurve densityCurve( const QMap<double, double> data,
                         const QString &name,
                         const QColor &lineColor,
                         const Qt::PenStyle penStyle,
                         const QBrush &brush)
{
    GraphCurve curve;
    curve.mName = name;
    if (!data.isEmpty()) {
        curve.mData = std::move(data);
        curve.mPen = QPen(lineColor, 1, penStyle);

        if (penStyle == Qt::CustomDashLine)
            curve.mPen.setDashPattern(QList<qreal>{5, 5});
        curve.mBrush = brush;
        curve.mIsRectFromZero = false; // for Unif-typo. calibs. and curveActivityUnifTheo, invisible for others!
        curve.mVisible = true;
    }
    return curve;
}
GraphCurve densityCurve( const std::map<double, double> data,
                        const QString &name,
                        const QColor &lineColor,
                        const Qt::PenStyle penStyle,
                        const QBrush &brush)
{
    GraphCurve curve;
    curve.mName = name;
    if (!data.empty()) {
        curve.mData = QMap(data);
        curve.mPen = QPen(lineColor, 1, penStyle);

        if (penStyle == Qt::CustomDashLine)
            curve.mPen.setDashPattern(QList<qreal>{5, 5});
        curve.mBrush = brush;
        curve.mIsRectFromZero = true; // for Unif-typo. calibs. and curveActivityUnifTheo, invisible for others!
        curve.mVisible = true;
    }
    return curve;
}

GraphCurve FunctionCurve(const QMap<double, double> data,
                   const QString &name,
                   const QColor &lineColor,
                   const Qt::PenStyle penStyle,
                   const QBrush &brush,
                   const bool is_visible)
{
    GraphCurve curve;
    curve.mName = name; // This is the name of the columns when exporting the graphs
    curve.mType = GraphCurve::eFunctionData;
    if (!data.isEmpty()) {
        curve.mData = std::move(data);
        curve.mPen = QPen(lineColor, 1, penStyle);

        if (penStyle == Qt::CustomDashLine)
            curve.mPen.setDashPattern(QList<qreal>{5, 5});
        curve.mBrush = brush;
        curve.mIsRectFromZero = false;
        curve.mVisible = is_visible;
    }
    return curve;
}

GraphCurve HPDCurve(QMap<double, double> data, const QString &name, const QColor &color, const bool is_visible)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mData = std::move(data);
    curve.mPen = Qt::NoPen;
    curve.mBrush = QBrush(color);
    curve.mIsRectFromZero = true;
    curve.mVisible = is_visible;
    return curve;
}

GraphCurve HPDCurve(std::map<double, double> data, const QString &name, const QColor &color, const bool is_visible)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mData = QMap(data);
    curve.mPen = Qt::NoPen;
    curve.mBrush = QBrush(color);
    curve.mIsRectFromZero = true;
    curve.mVisible = is_visible;
    return curve;
}

GraphCurve topLineSection(const std::pair<double, double> &section, const QString &name, const QColor &color)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mSections.push_back(section);
    curve.mPen.setColor(color);
    curve.mPen.setWidth(1);
    curve.mPen.setStyle(Qt::SolidLine);
    curve.mType = GraphCurve::eTopLineSections;
    curve.mVisible = true;
    return curve;
}

GraphCurve horizontalSection(const std::pair<double, double> &section, const QString &name, const QColor &color, const QBrush &brush)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mSections.push_back(section);
    curve.mBrush = brush;
    curve.mPen = QPen(QBrush(color), 2.);
    curve.mType = GraphCurve::eHorizontalSections;
    curve.mIsRectFromZero = true;
    return curve;
}

GraphCurve horizontalLine(const double yValue, const QString &name, const QColor &color,
                                                    const Qt::PenStyle penStyle)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mType = GraphCurve::eHorizontalLine;
    curve.mHorizontalValue = yValue;
    curve.mPen.setStyle(penStyle);
    curve.mPen.setColor(color);
    if (penStyle == Qt::CustomDashLine)
        curve.mPen.setDashPattern(QList<qreal>{5, 5});
    return curve;
}

GraphCurve shapeCurve(const QMap<double, double> &dataInf, const QMap<double, double> &dataSup,
                                                  const QString &name,
                                                  const QColor &lineColor,
                                                  const Qt::PenStyle penStyle,
                                                  const QBrush &brush,
                                                  const bool is_visible)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mType = GraphCurve::eShapeData;

    curve.mShape = std::make_pair(dataInf, dataSup);
    curve.mPen = QPen(lineColor, 1, penStyle);

    if (penStyle == Qt::CustomDashLine) {
        curve.mPen.setDashPattern(QList<qreal>{2, 2});
        /*QList<qreal> dashes;
        qreal space = 5;
        dashes << 5 << space << 5 << space << 9 << space<< 27 << space << 9 << space;
        curve.mPen.setDashPattern(dashes);*/
    }

    curve.mBrush = brush;
    curve.mIsRectFromZero = false; // for Unif-typo. calibs., invisible for others!
    curve.mVisible = is_visible;
    return curve;
}

GraphCurve shapeCurve(const std::map<double, double> &dataInf, const std::map<double, double> &dataSup,
                      const QString &name,
                      const QColor &lineColor,
                      const Qt::PenStyle penStyle,
                      const QBrush &brush,
                      const bool is_visible)
{
    GraphCurve curve;
    curve.mName = name;
    curve.mType = GraphCurve::eShapeData;

    const QMap<double, double> tmpInf (dataInf);
    const QMap<double, double> tmpSup (dataSup);
    curve.mShape = std::make_pair(tmpInf, tmpSup);
    curve.mPen = QPen(lineColor, 1, penStyle);

    if (penStyle == Qt::CustomDashLine) {
        curve.mPen.setDashPattern(QList<qreal>{2, 2});
        /*QList<qreal> dashes;
        qreal space = 5;
        dashes << 5 << space << 5 << space << 9 << space<< 27 << space << 9 << space;
        curve.mPen.setDashPattern(dashes);*/
    }

    curve.mBrush = brush;
    curve.mIsRectFromZero = false; // for Unif-typo. calibs., invisible for others!
    curve.mVisible = is_visible;
    return curve;
}
