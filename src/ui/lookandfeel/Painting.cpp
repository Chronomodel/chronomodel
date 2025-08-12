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

#include "Painting.h"

#include "QtUtilities.h"

#include <QApplication>
#include <QIcon>
#include <QPainter>
#include <QPalette>


QColor Painting::mainColorLight = QColor(49, 112, 176);
QColor Painting::mainColorDark = QColor(31, 80, 128);
QColor Painting::mainColorGrey = QColor(187, 187, 187);
QList<QColor> Painting::chainColors = QList<QColor>();
QColor Painting::greyedOut = QColor(255, 255, 255, 200);
QColor Painting::mainGreen = QColor(0, 169, 157);

QColor Painting::borderDark = QColor(50, 50, 50);

void Painting::init()
{
    chainColors.append(QColor(179, 70, 50)); // brown
    chainColors.append(QColor(74, 92, 164)); // blue
    chainColors.append(QColor(101, 154, 89)); // green
    chainColors.append(QColor(238, 50, 70)); // red
    chainColors.append(QColor(183, 158, 120)); // light brown
    chainColors.append(QColor(241, 36, 52)); // red
    chainColors.append(QColor(125, 144, 192)); // blue-grey
    chainColors.append(QColor(174, 32, 53)); // brown-bordeaux
    chainColors.append(QColor(26, 93, 75)); // dark-green
    chainColors.append(QColor(40, 33, 133)); // dark-blue

    for (int i = 0; i<200; ++i)
        chainColors.append(randomColor());
}


double pointSize(double size)
{
#if defined(QT_OS_MAC)

    return size;

#elif defined(QT_OS_WIN32) || defined(WIN32)

    return size * 72. / 96.;

#endif

    return size;
}

void drawButton(QPainter& painter, const QRectF& rect, bool hover, bool isEnabled, const QString& text, const QIcon& icon)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    if ( isEnabled && hover) {
        QLinearGradient grad(0, 0, 0, rect.height());
        grad.setColorAt(0, QColor(0, 0, 0));
        grad.setColorAt(1, QColor(20, 20, 20));
        painter.fillRect(rect, grad);

        painter.setPen(QColor(10, 10, 10));
        painter.drawLine(0, 0, rect.width(), 0);

        painter.setPen(QColor(30, 30, 30));
        painter.drawLine(0, rect.height(), rect.width(), rect.height());
    } else {
        QLinearGradient grad(0, 0, 0, rect.height());
        grad.setColorAt(0, QColor(40, 40, 40));
        grad.setColorAt(1, QColor(30, 30, 30));
        painter.fillRect(rect, grad);

        painter.setPen(Painting::borderDark);
        painter.drawLine(0, 0, rect.width(), 0);

        painter.setPen(Qt::black);
        painter.drawLine(0, rect.height(), rect.width(), rect.height());
    }

    int textH (22);

    QFont font = painter.font();
    painter.setFont(font);

    painter.setPen(QColor(200, 200, 200));
    painter.drawText(rect.adjusted(0, rect.height() - textH, 0, 0), Qt::AlignCenter, text);

    double m = 8.;
    double w = rect.width() - 2.*m;
    double h = rect.height() - m - textH;
    double s = qMin(w, h);

    QRectF iconRect((rect.width() - s)/2.f, m, s, s);
    QPixmap pixmap = icon.pixmap(iconRect.size().toSize());
    painter.drawPixmap(iconRect, pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));

    painter.restore();
}

void drawButton2(QPainter& painter, const QRectF& rect, bool hover, bool isEnabled, const QString& text, const QIcon& icon, bool isFlat)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect;

    if (isFlat) {
        painter.setPen(hover ? Qt::black : Painting::borderDark);
        painter.setBrush(hover ? Qt::black : Painting::borderDark);
        painter.drawRect(r);
    } else {
        r = rect.adjusted(1, 1, -1, -1);

        QLinearGradient grad(r.x(), r.y(), r.x(), r.y() + r.height());
        if (hover) {
            grad.setColorAt(0, QColor(48, 116, 159));
            grad.setColorAt(1, QColor(22, 70, 103));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 4, 4);

            grad.setColorAt(0, QColor(50, 82, 101));
            grad.setColorAt(1, QColor(40, 68, 82));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r.adjusted(2, 2, -2, -2), 2, 2);

            painter.setPen(QColor(27, 51, 59));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(r, 4, 4);

        } else if(!isEnabled) {
            painter.setPen(QColor(140, 140, 140));
            painter.setBrush(QColor(160, 160, 160));
            painter.drawRoundedRect(r, 4, 4);

        } else {
            grad.setColorAt(0, QColor(90, 90, 90));
            grad.setColorAt(1, QColor(70, 70, 70));
            painter.setBrush(grad);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(r, 4, 4);
            painter.setPen(QColor(110, 110, 110));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(r.adjusted(0, 1, 0, 0), 4, 4);
            painter.setPen(Painting::borderDark);
            painter.drawRoundedRect(r, 4, 4);
        }
    }

    QFont font = painter.font();
    painter.setFont(font);

    if (!icon.isNull()) {
        int s = (r.width() > r.height()) ? r.height() : r.width();
        int sm = 4;
        int gap = 5;

        if (!text.isEmpty()) {
            QFontMetrics metrics(painter.font());
            int mh = metrics.height();
            int h = r.height() - gap - mh;
            s = (r.width() > h) ? h : r.width();

            painter.setPen(hover ? Qt::white : QColor(200, 200, 200));
            painter.drawText(sm, r.height() - mh - sm, r.width() - 2*sm, mh, Qt::AlignCenter, text);
        }
        s -= 2*sm;
        QRect iconRect(r.x() + (r.width() - s)/2, r.y() + sm, s, s);
        painter.drawPixmap(iconRect, icon.pixmap(200, 200));
    }
    else if (!text.isEmpty()) {
        painter.setPen(hover ? Qt::white : QColor(200, 200, 200));
        painter.drawText(r, Qt::AlignCenter, text);
    }

    painter.restore();
}

void drawBox(QPainter& painter, const QRectF& r, const QString& text)
{
    painter.setPen(Painting::borderDark);
    painter.setBrush(QColor(75, 75, 75));
    painter.drawRect(r);
    painter.setBrush(Painting::borderDark);
    painter.drawRect(r.adjusted(0, 0, 0, -r.height() + 20));

    QFont font = painter.font();
    painter.setFont(font);

    painter.setPen(Qt::white);
    painter.drawText(r.adjusted(5, 0, -5, -r.height() + 20), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawRadio(QPainter& painter, const QRectF& rect, const QString& text, bool toggled)
{
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect.adjusted(1, 1, -1, -1);
    int subM (0);

    painter.setPen(QColor(120, 120, 120));
    painter.setBrush(QColor(230, 230, 230));
    painter.drawEllipse(r.adjusted(0, subM, r.height() - r.width() - 2*subM, -subM));

    if (toggled) {
        int insideM = 3;
        painter.setPen(Qt::NoPen);
        painter.setBrush(Painting::mainColorLight);
        painter.drawEllipse(r.adjusted(insideM,
                                       subM+insideM,
                                       r.height() - r.width() - 2*subM - insideM,
                                       -subM - insideM));
    }

    painter.setPen(qApp->palette().text().color());
    painter.drawText(r.adjusted(r.height() - 2*subM + 5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawCheckbox(QPainter& painter, const QRectF& r, const QString& text, Qt::CheckState state)
{
    painter.setRenderHint(QPainter::Antialiasing);

    int subM (0);

    QRectF boxRect = r.adjusted(0, subM, r.height() - r.width() - 2*subM, -subM);
    drawCheckBoxBox(painter, boxRect, state, QColor(230, 230, 230), QColor(120, 120, 120));

    painter.setPen( qApp->palette().text().color());

    painter.drawText(r.adjusted(r.height() - 2*subM + 5, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
}

void drawCheckBoxBox(QPainter& painter, const QRectF& rect, Qt::CheckState state, const QColor& back, const QColor& border)
{
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect.adjusted(1, 1, -1, -1);

    painter.setPen(border);
    painter.setBrush(back);
    painter.drawRect(r);

    QPen pen = painter.pen();
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setColor(Painting::mainColorLight);
    painter.setPen(pen);

    int mi = 2;
    QRectF lr = r.adjusted(mi, mi, -mi, -mi);

    if (state == Qt::Checked) {
        painter.drawLine(lr.x(), lr.y(), lr.x() + lr.width(), lr.y() + lr.height());
        painter.drawLine(lr.x() + lr.width(), lr.y(), lr.x(), lr.y() + lr.height());
    }
    else if (state == Qt::PartiallyChecked)
        painter.drawLine(lr.x(), lr.y() + lr.height()/2, lr.x() + lr.width(), lr.y() + lr.height()/2);

}

#pragma mark palette density
const std::vector<ColorStop>& ColorStops::getStops(ColorPalette palette)
{
    static const std::vector<ColorStop> BWStops = {
        {0.0, QColor(0, 0, 0)},       // Noir
        {1.0, QColor(255, 255, 255)}  // Blanc
    };

    static const std::vector<ColorStop> WBStops = {
        {0.0, QColor(255, 255, 255)},  // Blanc
        {1.0, QColor(0, 0, 0)}       // Noir
    };

    static const std::vector<ColorStop> pressureStops = {
        {0.0, QColor(0, 0, 139)}, // Bleu foncé
        {0.5, QColor(173, 216, 230)}, // Bleu claire
        {1.0, QColor(255, 0, 0)} // Rouge
    };

    static const std::vector<ColorStop> elevationStops = {
        {0.0, QColor(0, 100, 0)},     // Vert foncé (basses altitudes)
        {0.5, QColor(255, 255, 0)},   // Jaune (altitudes moyennes)
        {1.0, QColor(255, 255, 255)}  // Blanc (hautes altitudes)
    };

    // couleurs harmonieuses "Blues" de ColorBrewer //https://colorbrewer2.org/
    static const std::vector<ColorStop> BluesStops = {
        {0.0, QColor(8, 48, 107)},     // dark blue
        {0.25, QColor(33, 113, 181)}, // medium dark blue
        {0.5, QColor(189, 215, 231)},// light blue
        {0.75, QColor(189, 215, 231)},// light blue
        {1.0, QColor(247, 247, 247)}  // almost white
    };

    // "Inferno" (Matplotlib / Scientific colormaps)
    static const std::vector<ColorStop> infernoStops = {
        {0.0, QColor(0, 0, 4)},          // almost black
        {0.25, QColor(153, 28, 59)},     // deep burgundy
        {0.5, QColor(240, 96, 60)},      // deep orange
        {0.75, QColor(254, 204, 92)},    // golden yellow
        {1.0, QColor(252, 255, 164)}     // pale yellow
    };
    static const std::vector<ColorStop> infernoDensityStops = {
        {0.0, QColor(0, 0, 4, 0)},          // almost black
        {0.25, QColor(153, 28, 59, 63)},     // deep burgundy
        {0.5, QColor(240, 96, 60, 127)},      // deep orange
        {0.75, QColor(254, 204, 92, 191)},    // golden yellow
        {1.0, QColor(252, 255, 164, 255)}     // pale yellow
    };
    static const std::vector<ColorStop> geophyStops = {
        {0.0, QColor(161, 203, 148)}, // light green
        {0.25, QColor(189, 215, 231)}, // light blue
        {0.5, QColor(252, 255, 164)},     // pale yellow
        {0.75, QColor(254, 204, 92)},    // golden yellow
        {1.0, QColor(240, 96, 60)}      // deep orange
    };
    static const std::vector<ColorStop> geophyDensityStops = {
        {0.0, QColor(161, 203, 148, 0)}, // light green
        {0.25, QColor(189, 215, 23, 631)}, // light blue
        {0.5, QColor(252, 255, 164, 125)},     // pale yellow
        {0.75, QColor(254, 204, 92, 191)},    // golden yellow
        {1.0, QColor(240, 96, 60, 255)}      // deep orange
    };
    static const std::vector<ColorStop> temperatureStops = {
        {0.0, QColor(0, 0, 255)},     // Bleu froid (très basse température)
        {0.25, QColor(100, 149, 237)}, // Bleu clair
        {0.5, QColor(255, 255, 255)},  // Blanc (température neutre)
        {0.75, QColor(255, 165, 0)},   // Orange
        {1.0, QColor(255, 0, 0)}       // Rouge chaud (très haute température)
    };

    static const std::vector<ColorStop> temperatureSoftStops = {
        {0.0, QColor(0, 0, 255)},     // Bleu foncé
        {0.2, QColor(100, 149, 237)}, // Bleu clair
        {0.4, QColor(173, 216, 230)}, // Bleu très clair
        {0.5, QColor(255, 255, 255)}, // Blanc
        {0.6, QColor(255, 165, 0)},   // Orange
        {0.8, QColor(255, 69, 0)},    // Orange foncé
        {1.0, QColor(255, 0, 0)}      // Rouge vif
    };
    static const std::vector<ColorStop> temperatureSoftDensityStops = {
        {0.0, QColor(0, 0, 255, 0)},     // Bleu foncé
        {0.3, QColor(100, 149, 237, 76)}, // Bleu clair
        //{0.4, QColor(173, 216, 230, 102)}, // Bleu très clair
        //{0.5, QColor(255, 255, 255, 125)}, // Blanc
        {0.5, QColor(254, 204, 92, 125)}, // golden yellow

        {0.7, QColor(255, 165, 0, 178)},   // Orange
        //{0.8, QColor(255, 69, 0, 227)},    // Orange foncé
        {1.0, QColor(255, 0, 0, 255)}      // Rouge vif
    };
    static const std::vector<ColorStop> temperatureScienceStops = {
        {0.0, QColor(0, 0, 139, 120)},     // Bleu marine (très froid)
        {0.25, QColor(65, 105, 225, 120)}, // Bleu royal
        {0.5, QColor(135, 206, 235, 120)}, // Bleu ciel
        {0.75, QColor(255, 140, 0, 120)},  // Orange foncé
        {1.0, QColor(139, 0, 0, 120)}      // Rouge bordeaux (extrêmement chaud)
    };
    static const std::vector<ColorStop> probabilityDensityStops = {
        {0.0, QColor(240, 248, 255, 0)},     // Blanc bleuté transparent
        {0.2, QColor(176, 224, 230, 50)},    // Bleu pastel très transparent
        {0.5, QColor(135, 206, 235, 120)},   // Bleu ciel semi-transparent
        {0.75, QColor(70, 130, 180, 200)},   // Bleu acier plus opaque
        {1.0, QColor(25, 25, 112, 255)}      // Bleu nuit complètement opaque
    };

    static const std::vector<ColorStop> pHScaleStops = {
        {0.0/ 14.0, QColor(255, 0, 0, 125)},        // pH 0 - Rouge (acide fort)
        {1.0/ 14.0, QColor(255, 64, 0, 125)},       // pH 1 - Rouge orangé
        {2.0/ 14.0, QColor(255, 128, 0, 125)},      // pH 2 - Orange
        {3.0/ 14.0, QColor(255, 191, 0, 125)},      // pH 3 - Jaune orangé
        {4.0/ 14.0, QColor(255, 255, 0, 125)},      // pH 4 - Jaune
        {5.0/ 14.0, QColor(191, 255, 0, 125)},      // pH 5 - Vert jaunâtre
        {6.0/ 14.0, QColor(128, 255, 0, 125)},      // pH 6 - Vert clair
        {7.0/ 14.0, QColor(0, 255, 0, 125)},        // pH 7 - Vert (neutre)
        {8.0/ 14.0, QColor(0, 191, 255, 125)},      // pH 8 - Cyan
        {9.0/ 14.0, QColor(0, 128, 255, 125)},      // pH 9 - Bleu clair
        {10.0/ 14.0, QColor(0, 64, 255, 125)},      // pH 10 - Bleu
        {11.0/ 14.0, QColor(0, 0, 255, 125)},       // pH 11 - Bleu foncé
        {12.0/ 14.0, QColor(64, 0, 255, 125)},      // pH 12 - Violet
        {13.0/ 14.0, QColor(128, 0, 255, 125)},     // pH 13 - Violet clair
        {14.0/ 14.0, QColor(255, 0, 255, 125)}      // pH 14 - Magenta (basique fort)
    };
    static const std::vector<ColorStop> RHStops = {
        {0.0, QColor(141, 90, 44)},  // marron-terre
        {0.5, QColor(254, 254, 193)}, // jaune pale
        {1.0, QColor(71, 121, 124)}  // bleu foncé
    };
    static const std::vector<ColorStop> RHDensityStops = {
        {0.0, QColor(141, 90, 44, 0)},  // marron-terre
        {0.5, QColor(254, 254, 193, 127)}, // jaune pale
        {1.0, QColor(71, 121, 124, 255)}  // bleu foncé
    };
    switch (palette) {

    case ColorPalette::BlackWhite: return BWStops;
    case ColorPalette::WhiteBlack: return WBStops;
    case ColorPalette::Pressure: return pressureStops;
    case ColorPalette::Elevation: return elevationStops;
    case ColorPalette::Blues: return BluesStops;
    case ColorPalette::Inferno: return infernoStops;
    case ColorPalette::InfernoDensity: return infernoDensityStops;

    case ColorPalette::Geophy: return geophyStops;
    case ColorPalette::GeophyDensity: return geophyDensityStops;

    case ColorPalette::Temperature: return temperatureStops;
    case ColorPalette::TemperatureSoft: return temperatureSoftStops;
    case ColorPalette::TemperatureSoftDensity: return temperatureSoftDensityStops;
    case ColorPalette::TemperatureScience: return temperatureScienceStops;
    case ColorPalette::DataProbability: return probabilityDensityStops;

    case ColorPalette::pHScale: return  pHScaleStops;
    case ColorPalette::RH: return  RHStops;
    case ColorPalette::RHDensity: return  RHDensityStops;

    default: return BWStops;
    }
}

QColor ColorStops::getColorFromStops(double normVal, ColorPalette palette)
{
    const auto& stops = getStops(palette);
    normVal = std::clamp(normVal, 0.0, 1.0);

    for (size_t i = 0; i < stops.size() - 1; ++i) {
        if (normVal <= stops[i + 1].pos) {
            double ratio = (normVal - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            int r = static_cast<int>(stops[i].color.red()   * (1 - ratio) + stops[i + 1].color.red()   * ratio);
            int g = static_cast<int>(stops[i].color.green() * (1 - ratio) + stops[i + 1].color.green() * ratio);
            int b = static_cast<int>(stops[i].color.blue()  * (1 - ratio) + stops[i + 1].color.blue()  * ratio);
            int a = static_cast<int>(stops[i].color.alpha()  * (1 - ratio) + stops[i + 1].color.alpha()  * ratio);
            return QColor(r, g, b, a);
        }
    }

    return stops.back().color;
}

QColor ColorStops::getColorFromStops(double normVal, const std::vector<ColorStop> &stops)
{
    normVal = std::clamp(normVal, 0.0, 1.0);

    for (size_t i = 0; i < stops.size() - 1; ++i) {
        if (normVal <= stops[i + 1].pos) {
            double ratio = (normVal - stops[i].pos) / (stops[i + 1].pos - stops[i].pos);
            int r = static_cast<int>(stops[i].color.red()   * (1 - ratio) + stops[i + 1].color.red()   * ratio);
            int g = static_cast<int>(stops[i].color.green() * (1 - ratio) + stops[i + 1].color.green() * ratio);
            int b = static_cast<int>(stops[i].color.blue()  * (1 - ratio) + stops[i + 1].color.blue()  * ratio);
            int a = static_cast<int>(stops[i].color.alpha()  * (1 - ratio) + stops[i + 1].color.alpha()  * ratio);
            return QColor(r, g, b, a);
        }
    }

    return stops.back().color;
}
