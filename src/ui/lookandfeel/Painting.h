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

#ifndef PAINTING_H
#define PAINTING_H

#include <QList>
#include <QColor>
#include <QPainter>
#include <QIcon>


#define CURVE_COLOR_TEXT QColor(77, 37, 121)
#define CURVE_COLOR_BACK QColor(243, 231, 255)
#define CURVE_COLOR_BORDER QColor(77, 37, 121)

#define CHRONOMODEL_COLOR_TEXT QColor(40, 99, 157)
#define CHRONOMODEL_COLOR_BACK QColor(235, 248, 255)
#define CHRONOMODEL_COLOR_BORDER QColor(40, 99, 157)



class Painting
{
public:
    static void init();

    static QColor mainColorLight;
    static QColor mainColorDark;
    static QColor mainColorGrey;
    static QColor mainGreen;
    static QList<QColor> chainColors;
    static QColor greyedOut;
    static QColor borderDark;
};



double pointSize(double size);

void drawButton(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon());
void drawButton2(QPainter& painter, const QRectF& r, bool hover, bool isEnabled = true, const QString& text = QString(), const QIcon& icon = QIcon(), bool isFlat = false);
void drawBox(QPainter& painter, const QRectF& r, const QString& text);
//void drawRadio(QPainter& painter, const QRectF& r, const QString& text, bool toggled);
//void drawCheckbox(QPainter &painter, const QRectF &r, const QString& text, Qt::CheckState state);
void drawCheckBoxBox(QPainter& painter, const QRectF& r, Qt::CheckState state, const QColor& back, const QColor& border);

void drawRadio(QPainter& painter, const QRectF& rect, const QString& text, bool toggled,
               const QColor& dotColor = QColor());
void drawCheckbox(QPainter& painter, const QRectF& r, const QString& text, Qt::CheckState state,
                  const QColor& dotColor = QColor());

struct ColorStop {
    double pos;
    QColor color;
};

enum class ColorPalette {
    Greys = 0,
    BlackWhite = 1,

    Pressure = 2,
    Elevation = 3,
    Blues = 4,

    Geophy = 5,
    GeophyDensity = 6,


    Temperature = 7,
    TemperatureSoft = 8,
    TemperatureSoftDensity = 9,

    TemperatureScience = 10,
    DataProbability = 11,

    pHScale = 12,
    RH = 13,
    RHDensity = 14,
    // Palette Viridis
    Magma10 = 15,
    Inferno10 = 16,
    Plasma10 = 17,
    Viridis10 = 18,
    Cividis10 = 19,

    Magma10Density = 20,
    Inferno10Density = 21,
    Plasma10Density = 22,
    Viridis10Density = 23,
    Cividis10Density = 24,

    Spectral10Density = 25,
    Coolwarm10Density = 26,
    Bwr10Density = 27,

    Gist_rainbow14 = 28,
    Gist_rainbow14Density = 29

};

Q_DECLARE_METATYPE(ColorPalette)

class ColorStops {
public:
    static const std::vector<ColorStop>& getStops(ColorPalette palette);
    static QColor getColorFromStops(double normVal, ColorPalette palette);
    static QColor getColorFromStops(double normVal, const std::vector<ColorStop>& stops);
private:
    static constexpr int alpha_higthContrast = 150;
};

#endif
