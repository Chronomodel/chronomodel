/* ---------------------------------------------------------------------

Copyright or © or Copr. CNRS	2014 - 2026

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

#pragma once

// ---------------------------------------------------------------------
//   Includes
// ---------------------------------------------------------------------
#include <QWidget>
#include <QPainterPath>
#include <QtGlobal>          // pour qreal
#include <algorithm>         // std::clamp, std::max, std::lerp
#include <type_traits>       // std::is_floating_point_v

// ---------------------------------------------------------------------
//   Constantes (remplace les macros)
// ---------------------------------------------------------------------
inline constexpr qreal BLANK_SPACE_ON_TOP   = 5.0;   // espace réservé à la barre de crédibilité
inline constexpr qreal BLANK_SPACE_ON_RIGHT = 5.0;

// ---------------------------------------------------------------------
//   Types génériques
// ---------------------------------------------------------------------
using type_data = double;

// ---------------------------------------------------------------------
//   Fonctions utilitaires (inline, constexpr quand c’est possible)
// ---------------------------------------------------------------------
/**
 * @brief Retourne la valeur proportionnelle d’un paramètre.
 *
 * @tparam T   type numérique (float, double, …). Doit être un type flottant.
 * @param value          valeur à convertir
 * @param valMin,valMax   intervalle source
 * @param Pmin,Pmax       intervalle cible
 * @param resultInBounds  si true, le résultat est limité à [Pmin,Pmax]
 * @return valeur proportionnelle (ou bornée)
 */
template <typename T>
    requires std::is_floating_point_v<T>
inline constexpr T valueForProportion( T value,
                                      T valMin, T valMax,
                                      T Pmin,   T Pmax,
                                      bool resultInBounds )
{
    const T proportion = (value - valMin) / (valMax - valMin);
    const T v          = std::lerp( Pmin, Pmax, proportion );

    return resultInBounds ? std::clamp( v, Pmin, Pmax ) : v;
}

// ---------------------------------------------------------------------
//   Classe principale
// ---------------------------------------------------------------------
class GraphViewAbstract : public QWidget
{
    Q_OBJECT

public:
    // -----------------------------------------------------------------
    //   Construction / Destruction
    // -----------------------------------------------------------------
    explicit GraphViewAbstract( QWidget* parent = nullptr );
    virtual ~GraphViewAbstract() = default;

    // -----------------------------------------------------------------
    //   Getters (const, [[nodiscard]])
    // -----------------------------------------------------------------
    [[nodiscard]] inline type_data rangeX() const noexcept { return mMaxX - mMinX; }
    [[nodiscard]] inline type_data rangeY() const noexcept { return mMaxY - mMinY; }

    [[nodiscard]] inline type_data currentMinX() const noexcept { return mCurrentMinX; }
    [[nodiscard]] inline type_data currentMaxX() const noexcept { return mCurrentMaxX; }

    [[nodiscard]] inline type_data minimumX() const noexcept { return mMinX; }
    [[nodiscard]] inline type_data maximumX() const noexcept { return mMaxX; }
    [[nodiscard]] inline type_data minimumY() const noexcept { return mMinY; }
    [[nodiscard]] inline type_data maximumY() const noexcept { return mMaxY; }

    [[nodiscard]] inline qreal marginLeft()   const noexcept { return mMarginLeft;   }
    [[nodiscard]] inline qreal marginRight()  const noexcept { return mMarginRight;  }
    [[nodiscard]] inline qreal marginTop()    const noexcept { return mMarginTop;    }
    [[nodiscard]] inline qreal marginBottom() const noexcept { return mMarginBottom; }

    // -----------------------------------------------------------------
    //   Setters (inline, protection contre les changements inutiles)
    // -----------------------------------------------------------------
    inline void setGraphHeight( qreal h ) noexcept { mGraphHeight = h; }

    inline void setRangeX( type_data minX, type_data maxX ) noexcept
    {
        mMinX = minX;
        mMaxX = maxX;
    }

    inline void setCurrentX( type_data minX, type_data maxX ) noexcept
    {
        mCurrentMinX = minX;
        mCurrentMaxX = maxX;
    }

    /** @brief Définit la plage Y en évitant min == max et en vérifiant la cohérence. */
    inline void setRangeY( type_data minY, type_data maxY )
    {
        if ( minY == maxY ) {
            mMinY = minY - type_data(1.0);
            mMaxY = maxY + type_data(1.0);
        }
        else {
            mMinY = minY;
            mMaxY = maxY;
        }
    }

    // Setters simples (avec test d’égalité)
    inline void setMinimumX( type_data v ) noexcept { if (mMinX != v) mMinX = v; }
    inline void setMaximumX( type_data v ) noexcept { if (mMaxX != v) mMaxX = v; }
    inline void setMinimumY( type_data v ) noexcept { if (mMinY != v) mMinY = v; }
    inline void setMaximumY( type_data v ) noexcept { if (mMaxY != v) mMaxY = v; }

    inline void setMarginLeft  ( qreal v ) noexcept { if (mMarginLeft   != v) mMarginLeft   = v; }
    inline void setMarginRight ( qreal v ) noexcept { if (mMarginRight  != v) mMarginRight  = v; }
    inline void setMarginTop   ( qreal v ) noexcept { if (mMarginTop    != v) mMarginTop    = v; }
    inline void setMarginBottom( qreal v ) noexcept { if (mMarginBottom != v) mMarginBottom = v; }

    inline void setMargins( qreal left, qreal right, qreal top, qreal bottom ) noexcept
    {
        mMarginLeft   = left;
        mMarginRight  = right;
        mMarginTop    = top;
        mMarginBottom = bottom;
    }

    /** @brief Mémorise les paramètres courants afin de pouvoir détecter un changement. */
    inline void storeCurrentParameters() noexcept
    {
        mPrevMarginLeft   = mMarginLeft;
        mPrevMarginRight  = mMarginRight;
        mPrevMarginTop    = mMarginTop;
        mPrevMarginBottom = mMarginBottom;
        mPrevCurrentMinX  = mCurrentMinX;
        mPrevCurrentMaxX  = mCurrentMaxX;
        mPrevGraphWidth   = mGraphWidth;
        mPrevGraphHeight  = mGraphHeight;
    }

    /** @brief Indique si l’un des paramètres graphiques a changé depuis le dernier appel à `storeCurrentParameters()`. */
    [[nodiscard]] inline bool parametersChanged() const noexcept
    {
        const bool unchanged =
            (mMarginLeft   == mPrevMarginLeft)   && (mMarginRight  == mPrevMarginRight) &&
            (mMarginTop    == mPrevMarginTop)    && (mMarginBottom == mPrevMarginBottom) &&
            (mCurrentMinX  == mPrevCurrentMinX)  && (mCurrentMaxX  == mPrevCurrentMaxX) &&
            (mGraphWidth   == mPrevGraphWidth)   && (mGraphHeight  == mPrevGraphHeight);

        return !unchanged;
    }

protected:
    // -----------------------------------------------------------------
    //   Méthodes utilitaires de conversion (coordonnées ↔ valeurs)
    // -----------------------------------------------------------------
    /**
     * @brief Convertit une valeur de donnée en position X du graphe.
     * @param value          valeur à convertir
     * @param constrainResult si true, la position est bornée dans le cadre du graphe
     */
    [[nodiscard]] inline qreal getXForValue( type_data value,
                                            bool constrainResult = true ) const noexcept
    {
        return mMarginLeft + valueForProportion( value,
                                                mCurrentMinX,
                                                mCurrentMaxX,
                                                0.0,
                                                std::max( 0.0, mGraphWidth - BLANK_SPACE_ON_RIGHT ),
                                                constrainResult );
    }

    [[nodiscard]] inline type_data getValueForX( qreal x,
                                                bool constrainResult = true ) const noexcept
    {
        const qreal xFromSide = x - mMarginLeft;
        return valueForProportion( static_cast<type_data>(xFromSide),
                                  0.0,
                                  std::max( 0.0, mGraphWidth - BLANK_SPACE_ON_RIGHT ),
                                  mCurrentMinX,
                                  mCurrentMaxX,
                                  constrainResult );
    }

    [[nodiscard]] inline qreal getYForValue( type_data value,
                                            bool constrainResult = true ) const noexcept
    {
        const type_data yFromBase = valueForProportion( value,
                                                       mMinY,
                                                       mMaxY,
                                                       0.0,
                                                       std::max( 0.0, static_cast<type_data>(mGraphHeight) - BLANK_SPACE_ON_TOP ),
                                                       constrainResult );
        return mGraphHeight + mMarginTop - static_cast<qreal>(yFromBase);
    }

    [[nodiscard]] inline type_data getValueForY( qreal y,
                                                bool constrainResult = true ) const noexcept
    {
        const qreal yFromBase = mMarginTop + mGraphHeight - y;
        return valueForProportion( static_cast<type_data>(yFromBase),
                                  0.0,
                                  std::max( 0.0, static_cast<type_data>(mGraphHeight) - BLANK_SPACE_ON_TOP ),
                                  mMinY,
                                  mMaxY,
                                  constrainResult );
    }

    // -----------------------------------------------------------------
    //   Méthode pure à implémenter par les classes dérivées
    // -----------------------------------------------------------------
    virtual void repaintGraph() = 0;

    // -----------------------------------------------------------------
    //   Données membres (protected → accessibles aux classes dérivées)
    // -----------------------------------------------------------------
    QPainterPath mPainterPath;

    // Dimensions du graphe (en pixels)
    qreal mGraphWidth  = 0.0;
    qreal mGraphHeight = 0.0;

    // Marges autour du graphe
    qreal mMarginLeft   = 0.0;
    qreal mMarginRight  = 0.0;
    qreal mMarginTop    = 0.0;
    qreal mMarginBottom = 0.0;

    // Plages de données (valeurs réelles)
    type_data mMinX = 0.0, mMaxX = 0.0;
    type_data mMinY = 0.0, mMaxY = 0.0;

    // Plage affichée courante (utile pour le zoom/pan)
    type_data mCurrentMinX = 0.0, mCurrentMaxX = 0.0;

    // -----------------------------------------------------------------
    //   Sauvegarde des paramètres précédents (pour détecter les changements)
    // -----------------------------------------------------------------
    qreal      mPrevGraphWidth   = 0.0;
    qreal      mPrevGraphHeight  = 0.0;
    qreal      mPrevMarginLeft   = 0.0;
    qreal      mPrevMarginRight  = 0.0;
    qreal      mPrevMarginTop    = 0.0;
    qreal      mPrevMarginBottom = 0.0;
    type_data  mPrevCurrentMinX  = 0.0;
    type_data  mPrevCurrentMaxX  = 0.0;
};

