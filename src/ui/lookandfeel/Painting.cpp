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
        {0.0, QColor(0, 0, 0, alpha_higthContrast)},       // Noir
        {1.0, QColor(255, 255, 255, alpha_higthContrast)}  // Blanc
    };

    static const std::vector<ColorStop> GreysStops = {
        {0.0, QColor(255, 255, 255, alpha_higthContrast)},  // Blanc
        {1.0, QColor(0, 0, 0, alpha_higthContrast)}       // Noir
    };

    static const std::vector<ColorStop> pressureStops = {
        {0.0, QColor(0, 0, 139, alpha_higthContrast)}, // Bleu foncé
        {0.5, QColor(173, 216, 230, alpha_higthContrast)}, // Bleu claire
        {1.0, QColor(255, 0, 0, alpha_higthContrast)} // Rouge
    };

    static const std::vector<ColorStop> elevationStops = {
        {0.0, QColor(0, 100, 0, alpha_higthContrast)},     // Vert foncé (basses altitudes)
        {0.5, QColor(255, 255, 0, alpha_higthContrast)},   // Jaune (altitudes moyennes)
        {1.0, QColor(255, 255, 255, alpha_higthContrast)}  // Blanc (hautes altitudes)
    };

    // couleurs harmonieuses "Blues" de ColorBrewer //https://colorbrewer2.org/
    static const std::vector<ColorStop> BluesStops = {
        {0.0, QColor(8, 48, 107, alpha_higthContrast)},     // dark blue
        {0.25, QColor(33, 113, 181, alpha_higthContrast)}, // medium dark blue
        {0.5, QColor(189, 215, 231, alpha_higthContrast)},// light blue
        {0.75, QColor(189, 215, 231, alpha_higthContrast)},// light blue
        {1.0, QColor(247, 247, 247, alpha_higthContrast)}  // almost white
    };

    static const std::vector<ColorStop> geophyStops = {
        {0.0, QColor(161, 203, 148, alpha_higthContrast)}, // light green
        {0.25, QColor(189, 215, 231, alpha_higthContrast)}, // light blue
        {0.5, QColor(252, 255, 164, alpha_higthContrast)},     // pale yellow
        {0.75, QColor(254, 204, 92, alpha_higthContrast)},    // golden yellow
        {1.0, QColor(240, 96, 60, alpha_higthContrast)}      // deep orange
    };
    static const std::vector<ColorStop> geophyDensityStops = {
        {0.0, QColor(161, 203, 148, 0)}, // light green
        {0.25, QColor(189, 215, 23, 631)}, // light blue
        {0.5, QColor(252, 255, 164, 125)},     // pale yellow
        {0.75, QColor(254, 204, 92, 191)},    // golden yellow
        {1.0, QColor(240, 96, 60, 255)}      // deep orange
    };
    static const std::vector<ColorStop> temperatureStops = {
        {0.0, QColor(0, 0, 255, alpha_higthContrast)},     // Bleu froid (très basse température)
        {0.25, QColor(100, 149, 237, alpha_higthContrast)}, // Bleu clair
        {0.5, QColor(255, 255, 255, alpha_higthContrast)},  // Blanc (température neutre)
        {0.75, QColor(255, 165, 0, alpha_higthContrast)},   // Orange
        {1.0, QColor(255, 0, 0, alpha_higthContrast)}       // Rouge chaud (très haute température)
    };

    static const std::vector<ColorStop> temperatureSoftStops = {
        {0.0, QColor(0, 0, 255, alpha_higthContrast)},     // Bleu foncé
        {0.2, QColor(100, 149, 237, alpha_higthContrast)}, // Bleu clair
        {0.4, QColor(173, 216, 230, alpha_higthContrast)}, // Bleu très clair
        {0.5, QColor(255, 255, 255, alpha_higthContrast)}, // Blanc
        {0.6, QColor(255, 165, 0, alpha_higthContrast)},   // Orange
        {0.8, QColor(255, 69, 0, alpha_higthContrast)},    // Orange foncé
        {1.0, QColor(255, 0, 0, alpha_higthContrast)}      // Rouge vif
    };
    static const std::vector<ColorStop> temperatureSoftDensityStops = {
        {0.0, QColor(0, 0, 255, 0)},     // Bleu foncé
        {0.25, QColor(100, 149, 237, 76)}, // Bleu clair
        //{0.4, QColor(173, 216, 230, 102)}, // Bleu très clair
        //{0.5, QColor(255, 255, 255, 125)}, // Blanc
        {0.5, QColor(254, 204, 92, 125)}, // golden yellow

        {0.75, QColor(255, 165, 0, 178)},   // Orange
        //{0.8, QColor(255, 69, 0, 227)},    // Orange foncé
        {1.0, QColor(255, 0, 0, 255)}      // Rouge vif
    };
    static const std::vector<ColorStop> temperatureScienceStops = {
        {0.0, QColor(0, 0, 139, alpha_higthContrast)},     // Bleu marine (très froid)
        {0.25, QColor(65, 105, 225, alpha_higthContrast)}, // Bleu royal
        {0.5, QColor(135, 206, 235, alpha_higthContrast)}, // Bleu ciel
        {0.75, QColor(255, 140, 0, alpha_higthContrast)},  // Orange foncé
        {1.0, QColor(139, 0, 0, alpha_higthContrast)}      // Rouge bordeaux (extrêmement chaud)
    };
    static const std::vector<ColorStop> probabilityDensityStops = {
        {0.0, QColor(240, 248, 255, 0)},     // Blanc bleuté transparent
        {0.2, QColor(176, 224, 230, 50)},    // Bleu pastel très transparent
        {0.5, QColor(135, 206, 235, 120)},   // Bleu ciel semi-transparent
        {0.75, QColor(70, 130, 180, 200)},   // Bleu acier plus opaque
        {1.0, QColor(25, 25, 112, 255)}      // Bleu nuit complètement opaque
    };

    static const std::vector<ColorStop> pHScaleStops = {
        {0.0/ 14.0, QColor(255, 0, 0, alpha_higthContrast)},        // pH 0 - Rouge (acide fort)
        {1.0/ 14.0, QColor(255, 64, 0, alpha_higthContrast)},       // pH 1 - Rouge orangé
        {2.0/ 14.0, QColor(255, 128, 0, alpha_higthContrast)},      // pH 2 - Orange
        {3.0/ 14.0, QColor(255, 191, 0, alpha_higthContrast)},      // pH 3 - Jaune orangé
        {4.0/ 14.0, QColor(255, 255, 0, alpha_higthContrast)},      // pH 4 - Jaune
        {5.0/ 14.0, QColor(191, 255, 0, alpha_higthContrast)},      // pH 5 - Vert jaunâtre
        {6.0/ 14.0, QColor(128, 255, 0, alpha_higthContrast)},      // pH 6 - Vert clair
        {7.0/ 14.0, QColor(0, 255, 0, alpha_higthContrast)},        // pH 7 - Vert (neutre)
        {8.0/ 14.0, QColor(0, 191, 255, alpha_higthContrast)},      // pH 8 - Cyan
        {9.0/ 14.0, QColor(0, 128, 255, alpha_higthContrast)},      // pH 9 - Bleu clair
        {10.0/ 14.0, QColor(0, 64, 255, alpha_higthContrast)},      // pH 10 - Bleu
        {11.0/ 14.0, QColor(0, 0, 255, alpha_higthContrast)},       // pH 11 - Bleu foncé
        {12.0/ 14.0, QColor(64, 0, 255, alpha_higthContrast)},      // pH 12 - Violet
        {13.0/ 14.0, QColor(128, 0, 255, alpha_higthContrast)},     // pH 13 - Violet clair
        {14.0/ 14.0, QColor(255, 0, 255, alpha_higthContrast)}      // pH 14 - Magenta (basique fort)
    };

    static const std::vector<ColorStop> RHStops = {
        {0.0, QColor(141, 90, 44, alpha_higthContrast)},  // marron-terre
        {0.5, QColor(254, 254, 193, alpha_higthContrast)}, // jaune pale
        {1.0, QColor(71, 121, 124, alpha_higthContrast)}  // bleu foncé
    };
    static const std::vector<ColorStop> RHDensityStops = {
        {0.0, QColor(141, 90, 44, 0)},  // marron-terre
        {0.5, QColor(254, 254, 193, 127)}, // jaune pale
        {1.0, QColor(71, 121, 124, 255)}  // bleu foncé
    };

    // "Inferno" (Matplotlib / Scientific colormaps)
    static const std::vector<ColorStop> infernoStops = {
        {0.0, QColor(0, 0, 4, alpha_higthContrast)},          // almost black
        {0.25, QColor(153, 28, 59, alpha_higthContrast)},     // deep burgundy
        {0.5, QColor(240, 96, 60, alpha_higthContrast)},      // deep orange
        {0.75, QColor(254, 204, 92, alpha_higthContrast)},    // golden yellow
        {1.0, QColor(252, 255, 164, alpha_higthContrast)}     // pale yellow
    };
    static const std::vector<ColorStop> infernoDensityStops = {
        {0.0, QColor(0, 0, 4, 0)},          // almost black
        {0.25, QColor(153, 28, 59, 63)},     // deep burgundy
        {0.5, QColor(240, 96, 60, 127)},      // deep orange
        {0.75, QColor(254, 204, 92, 191)},    // golden yellow
        {1.0, QColor(252, 255, 164, 255)}     // pale yellow
    };

    // Palette Viridis
    static const std::vector<ColorStop> magma10 = {
        {0.0, QColor(  0,  0,  4, alpha_higthContrast)},
        {0.1, QColor( 28, 16, 68, alpha_higthContrast)},
        {0.2, QColor( 79, 18,123, alpha_higthContrast)},
        {0.3, QColor(129, 37,129, alpha_higthContrast)},
        {0.4, QColor(181, 54,122, alpha_higthContrast)},
        {0.5, QColor(229, 80,100, alpha_higthContrast)},
        {0.6, QColor(251,135, 97, alpha_higthContrast)},
        {0.7, QColor(254,194,135, alpha_higthContrast)},
        {0.8, QColor(253,236,178, alpha_higthContrast)},
        {0.9, QColor(251,254,214, alpha_higthContrast)},
        {1.0, QColor(252,253,253, alpha_higthContrast)}
    };
    static const std::vector<ColorStop> inferno10 = {
        {0.0, QColor(  0,  0,  4, alpha_higthContrast)},
        {0.1, QColor( 31, 12, 72, alpha_higthContrast)},
        {0.2, QColor( 85, 15,109, alpha_higthContrast)},
        {0.3, QColor(136, 34,106, alpha_higthContrast)},
        {0.4, QColor(186, 54, 85, alpha_higthContrast)},
        {0.5, QColor(227, 89, 51, alpha_higthContrast)},
        {0.6, QColor(249,140, 10, alpha_higthContrast)},
        {0.7, QColor(253,186, 45, alpha_higthContrast)},
        {0.8, QColor(253,224,124, alpha_higthContrast)},
        {0.9, QColor(253,245,185, alpha_higthContrast)},
        {1.0, QColor(252,255,233, alpha_higthContrast)}
    };

    static const std::vector<ColorStop> plasma10 = {
        {0.0, QColor( 13,  8,135, alpha_higthContrast)},
        {0.1, QColor( 63,  3,168, alpha_higthContrast)},
        {0.2, QColor(110,  1,179, alpha_higthContrast)},
        {0.3, QColor(158, 23,176, alpha_higthContrast)},
        {0.4, QColor(203, 54,155, alpha_higthContrast)},
        {0.5, QColor(238, 93,120, alpha_higthContrast)},
        {0.6, QColor(254,141, 82, alpha_higthContrast)},
        {0.7, QColor(252,191, 55, alpha_higthContrast)},
        {0.8, QColor(234,231, 44, alpha_higthContrast)},
        {0.9, QColor(195,254, 62, alpha_higthContrast)},
        {1.0, QColor(240,249, 33, alpha_higthContrast)}
    };
    static const std::vector<ColorStop> cividis10 = {
        {0.0, QColor(  0, 32, 76, alpha_higthContrast)},
        {0.1, QColor( 25, 46,102, alpha_higthContrast)},
        {0.2, QColor( 44, 60,124, alpha_higthContrast)},
        {0.3, QColor( 63, 75,136, alpha_higthContrast)},
        {0.4, QColor( 85, 90,140, alpha_higthContrast)},
        {0.5, QColor(110,105,138, alpha_higthContrast)},
        {0.6, QColor(137,121,130, alpha_higthContrast)},
        {0.7, QColor(165,137,115, alpha_higthContrast)},
        {0.8, QColor(194,152, 93, alpha_higthContrast)},
        {0.9, QColor(223,167, 59, alpha_higthContrast)},
        {1.0, QColor(250,181,  0, alpha_higthContrast)}
    };
    static const std::vector<ColorStop> viridis10 = {
        {0.0, QColor( 68,   1,  84, alpha_higthContrast)},
        {0.1, QColor( 72,  40, 120, alpha_higthContrast)},
        {0.2, QColor( 62,  74, 137, alpha_higthContrast)},
        {0.3, QColor( 49, 104, 142, alpha_higthContrast)},
        {0.4, QColor( 38, 130, 142, alpha_higthContrast)},
        {0.5, QColor( 31, 158, 137, alpha_higthContrast)},
        {0.6, QColor( 53, 183, 121, alpha_higthContrast)},
        {0.7, QColor(110, 206,  88, alpha_higthContrast)},
        {0.8, QColor(181, 222,  43, alpha_higthContrast)},
        {0.9, QColor(253, 231,  37, alpha_higthContrast)},
        {1.0, QColor(253, 231,  36, alpha_higthContrast)}
    };
    // versio avec variation de alpha-densité
    static const std::vector<ColorStop> magma10density = {
        {0.0, QColor(  0,  0,   4,  10)},
        {0.1, QColor( 28, 16,  68,  34)},
        {0.2, QColor( 79, 18, 123,  59)},
        {0.3, QColor(129, 37, 129,  84)},
        {0.4, QColor(181, 54, 122, 108)},
        {0.5, QColor(229, 80, 100, 133)},
        {0.6, QColor(251,135,  97, 158)},
        {0.7, QColor(254,194, 135, 183)},
        {0.8, QColor(253,236, 178, 207)},
        {0.9, QColor(251,254, 214, 232)},
        {1.0, QColor(252,253, 253, 255)}
    };
    static const std::vector<ColorStop> inferno10density = {
        {0.0, QColor(  0,  0,   4,  10)},
        {0.1, QColor( 31, 12,  72,  34)},
        {0.2, QColor( 85, 15, 109,  59)},
        {0.3, QColor(136, 34, 106,  84)},
        {0.4, QColor(186, 54,  85, 108)},
        {0.5, QColor(227, 89,  51, 133)},
        {0.6, QColor(249,140,  10, 158)},
        {0.7, QColor(253,186,  45, 183)},
        {0.8, QColor(253,224, 124, 207)},
        {0.9, QColor(253,245, 185, 232)},
        {1.0, QColor(252,255, 233, 255)}
    };
    static const std::vector<ColorStop> plasma10density = {
        {0.0, QColor( 13,  8, 135,  10)},
        {0.1, QColor( 63,  3, 168,  34)},
        {0.2, QColor(110,  1, 179,  59)},
        {0.3, QColor(158, 23, 176,  84)},
        {0.4, QColor(203, 54, 155, 108)},
        {0.5, QColor(238, 93, 120, 133)},
        {0.6, QColor(254,141,  82, 158)},
        {0.7, QColor(252,191,  55, 183)},
        {0.8, QColor(234,231,  44, 207)},
        {0.9, QColor(195,254,  62, 232)},
        {1.0, QColor(240,249,  33, 255)}
    };
    static const std::vector<ColorStop> viridis10density = {
        {0.0, QColor( 68,   1,  84,  10)},
        {0.1, QColor( 72,  40, 120,  34)},
        {0.2, QColor( 62,  74, 137,  59)},
        {0.3, QColor( 49, 104, 142,  84)},
        {0.4, QColor( 38, 130, 142, 108)},
        {0.5, QColor( 31, 158, 137, 133)},
        {0.6, QColor( 53, 183, 121, 158)},
        {0.7, QColor(110, 206,  88, 183)},
        {0.8, QColor(181, 222,  43, 207)},
        {0.9, QColor(253, 231,  37, 232)},
        {1.0, QColor(253, 231,  36, 255)}
    };
    static const std::vector<ColorStop> cividis10density = {
        {0.0, QColor(  0, 32,  76,  10)},
        {0.1, QColor( 25, 46, 102,  34)},
        {0.2, QColor( 44, 60, 124,  59)},
        {0.3, QColor( 63, 75, 136,  84)},
        {0.4, QColor( 85, 90, 140, 108)},
        {0.5, QColor(110,105, 138, 133)},
        {0.6, QColor(137,121, 130, 158)},
        {0.7, QColor(165,137, 115, 183)},
        {0.8, QColor(194,152,  93, 207)},
        {0.9, QColor(223,167,  59, 232)},
        {1.0, QColor(250,181,   0, 255)}
    };

    // Diverging
    static const std::vector<ColorStop> spectral10density = {
        {0.0, QColor(158,   1,  66,  10)},
        {0.1, QColor(213,  62,  79,  34)},
        {0.2, QColor(244, 109,  67,  58)},
        {0.3, QColor(253, 174,  97,  83)},
        {0.4, QColor(254, 224, 139, 107)},
        {0.5, QColor(255, 255, 191, 131)},
        {0.6, QColor(230, 245, 152, 156)},
        {0.7, QColor(171, 221, 164, 180)},
        {0.8, QColor(102, 194, 165, 204)},
        {0.9, QColor( 50, 136, 189, 229)},
        {1.0, QColor( 94,  79, 162, 255)}
    };
    static const std::vector<ColorStop> coolwarm10density = {
        {0.0, QColor( 59,  76, 192,  10)},
        {0.1, QColor( 68,  90, 204,  34)},
        {0.2, QColor( 77, 104, 215,  58)},
        {0.3, QColor( 87, 117, 225,  83)},
        {0.4, QColor(120, 150, 237, 107)},
        {0.5, QColor(181, 180, 180, 131)}, // point neutre
        {0.6, QColor(220, 143, 110, 156)},
        {0.7, QColor(231, 125,  88, 180)},
        {0.8, QColor(222,  96,  57, 204)},
        {0.9, QColor(204,  71,  37, 229)},
        {1.0, QColor(180,   4,  38, 255)}
    };
    static const std::vector<ColorStop> bwr10density = {
        {0.0, QColor(  0,   0, 255,  10)},
        {0.1, QColor( 51,  51, 255,  34)},
        {0.2, QColor(102, 102, 255,  58)},
        {0.3, QColor(153, 153, 255,  83)},
        {0.4, QColor(204, 204, 255, 107)},
        {0.5, QColor(255, 255, 255, 131)}, // blanc au centre
        {0.6, QColor(255, 204, 204, 156)},
        {0.7, QColor(255, 153, 153, 180)},
        {0.8, QColor(255, 102, 102, 204)},
        {0.9, QColor(255,  51,  51, 229)},
        {1.0, QColor(255,   0,   0, 255)}
    };

    // Miscellaneous

    static const std::vector<ColorStop> gist_rainbow14 = {
        {0.0,  QColor( 63,   0, 255, alpha_higthContrast)},
        {0.083, QColor(  0,  63, 255, alpha_higthContrast)},
        {0.167, QColor(  0, 127, 255, alpha_higthContrast)},
        {0.25,  QColor(  0, 191, 255, alpha_higthContrast)},
        {0.333, QColor(  0, 255, 191, alpha_higthContrast)},
        {0.417, QColor(  0, 255, 127, alpha_higthContrast)},
        {0.5,   QColor(  0, 255,  63, alpha_higthContrast)},
        {0.583, QColor( 63, 255,   0, alpha_higthContrast)},
        {0.667, QColor(127, 255,   0, alpha_higthContrast)},
        {0.75,  QColor(191, 255,   0, alpha_higthContrast)},
        {0.833, QColor(255, 191,   0, alpha_higthContrast)},
        {0.917, QColor(255, 127,   0, alpha_higthContrast)},
        {1.0,   QColor(255,   0,   0, alpha_higthContrast)}
    };

    static const std::vector<ColorStop> gist_rainbow14density = {
        {0.0,  QColor( 63,   0, 255, 10)},
        {0.083, QColor(  0,  63, 255, 30)},
        {0.167, QColor(  0, 127, 255, 51)},
        {0.25,  QColor(  0, 191, 255, 71)},
        {0.333, QColor(  0, 255, 191, 92)},
        {0.417, QColor(  0, 255, 127, 112)},
        {0.5,   QColor(  0, 255,  63, 133)},
        {0.583, QColor( 63, 255,   0, 153)},
        {0.667, QColor(127, 255,   0, 174)},
        {0.75,  QColor(191, 255,   0, 194)},
        {0.833, QColor(255, 191,   0, 215)},
        {0.917, QColor(255, 127,   0, 235)},
        {1.0,   QColor(255,   0,   0, 255)}
    };




    switch (palette) {

    case ColorPalette::BlackWhite: return BWStops;
    case ColorPalette::Greys: return GreysStops;
    case ColorPalette::Pressure: return pressureStops;
    case ColorPalette::Elevation: return elevationStops;
    case ColorPalette::Blues: return BluesStops;


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

    // Palette Viridis
    case ColorPalette::Magma10: return magma10; // A
    case ColorPalette::Inferno10: return inferno10; // B
    case ColorPalette::Plasma10: return plasma10; // C
    case ColorPalette::Viridis10: return viridis10; // D
    case ColorPalette::Cividis10: return cividis10; // E

    // Palette Viridis avec variation d'alpha
    case ColorPalette::Magma10Density: return magma10density; // A
    case ColorPalette::Inferno10Density: return inferno10density; // B
    case ColorPalette::Plasma10Density: return plasma10density; // C
    case ColorPalette::Viridis10Density: return viridis10density; // D
    case ColorPalette::Cividis10Density: return cividis10density; // E

    case ColorPalette::Spectral10Density: return spectral10density;
    case ColorPalette::Coolwarm10Density: return coolwarm10density;
    case ColorPalette::Bwr10Density: return bwr10density;

    case ColorPalette::Gist_rainbow14: return gist_rainbow14;
    case ColorPalette::Gist_rainbow14Density: return gist_rainbow14density;
    default: return temperatureSoftDensityStops;
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
