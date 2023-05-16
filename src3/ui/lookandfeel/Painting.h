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

#ifndef PAINTING_H
#define PAINTING_H

#include <QList>
#include <QtWidgets>

//#define Curve_COLOR_TEXT QColor(44, 122, 123)
//#define Curve_COLOR_BACK QColor(230, 255, 250)
//#define Curve_COLOR_BORDER QColor(44, 122, 123)

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
void drawRadio(QPainter& painter, const QRectF& r, const QString& text, bool toggled);
void drawCheckbox(QPainter &painter, const QRectF &r, const QString& text, Qt::CheckState state);
void drawCheckBoxBox(QPainter& painter, const QRectF& r, Qt::CheckState state, const QColor& back, const QColor& border);


#endif
