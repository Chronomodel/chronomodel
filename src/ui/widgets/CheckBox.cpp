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

#include "CheckBox.h"
#include "Painting.h"
#include "AppSettings.h"

#include <QtWidgets>

CheckBox::CheckBox(QWidget* parent):
    QCheckBox(parent)
{
    setCursor(Qt::PointingHandCursor);
}

CheckBox::CheckBox(const QString& text, QWidget* parent):
    QCheckBox(text, parent)
{
    setCursor(Qt::PointingHandCursor);
}

CheckBox::~CheckBox()
{

}

bool CheckBox::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        // Infobulles standard Qt, actives uniquement si l'aide de l'application est activée
        if (AppSettings::mShowHelp && !toolTip().isEmpty()) {
            QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), toolTip(), this);
        } else {
            QToolTip::hideText();
            event->ignore();
        }
        return true;
    }
    return QCheckBox::event(event);
}

void CheckBox::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    drawCheckbox(p, rect(), text(), checkState(), mHasDot ? mDotColor : QColor());
}