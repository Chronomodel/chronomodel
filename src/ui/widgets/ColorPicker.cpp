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

#include "ColorPicker.h"

#include <QColorDialog>
#include <QApplication>
#include <QPainter>
#include <QPainter>
#include <QPainterPath>

Q_PROPERTY(QColor mColor READ getColor WRITE setColor)

ColorPicker::ColorPicker(const QColor &color, QWidget* parent, Qt::WindowFlags flags):
    QWidget(parent, flags),
    mColor(color)
{
    setFixedHeight(20);
}

ColorPicker::~ColorPicker()
{

}

void ColorPicker::setColor(const QColor& color)
{
    mColor = color;
    update();
}

QColor ColorPicker::getColor() const
{
    return mColor;
}

void ColorPicker::openDialog()
{
    QColor color = QColorDialog::getColor(mColor, qApp->activeWindow(), tr("Select Colour"));
    if (color.isValid()) {
        setColor(color);
        emit colorChanged(color);
    }
}

void ColorPicker::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1,  -1, - 1), 4, 4);
    painter.fillPath(path, isEnabled() ? mColor : QColor(220, 220, 220));
    painter.strokePath(path, QColor(160, 160, 160));
}

void ColorPicker::mousePressEvent(QMouseEvent* e)
{
    Q_UNUSED(e);
    if (isEnabled())
        openDialog();

}
