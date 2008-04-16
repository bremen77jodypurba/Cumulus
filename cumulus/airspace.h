/***********************************************************************
**
**   airspace.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2000 by Heiner Lamprecht, Florian Ehinger
**                   2008 Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef AIRSPACE_H
#define AIRSPACE_H

#include <QDateTime>
#include <QPolygon>
#include <QPainter>
#include <QRect>

#include "altitude.h"
#include "lineelement.h"

/**
  * @short Collection of distances to airspaces
  *
  * This class holds a set of six distances to airspaces, used to warn the user if he's getting
  * (too) close to an airspace.
  *
  * @author André Somers
  */
struct AirspaceWarningDistance
{
    Distance horClose;
    Distance horVeryClose;
    Distance verAboveClose;
    Distance verAboveVeryClose;
    Distance verBelowClose;
    Distance verBelowVeryClose;

    bool operator==(const AirspaceWarningDistance& x) const {
        return (
                horClose == x.horClose &&
                horVeryClose == x.horVeryClose &&
                verAboveClose == x.verAboveClose &&
                verAboveVeryClose == x.verAboveVeryClose &&
                verBelowClose == x.verBelowClose &&
                verBelowVeryClose == x.verBelowVeryClose
               );
    }

    bool operator!=(const AirspaceWarningDistance& x) const {
        return !operator==(x);
    }
};


class AirRegion;

/**
 * @short Class to handle airspaces.
 *
 * This class is used for the several airspaces. The object can be
 * one of: AirC, AirCtemp, AirD, AirDtemp, ControlD, AirElow, AirEhigh,
 * AirF, Restricted, Danger, LowFlight
 * @author Heiner Lamprecht, Florian Ehinger
 * @version $Id$
 * @see BaseMapElement#objectType
 */
class Airspace : public LineElement
{
public:
    enum ConflictType {none=0, near=1, veryNear=2, inside=3};

    /**
     * Creates a new Airspace-object. n is the name, t the typeID. length
     * is the number of coordinates. upper and upperType give the upper limit
     * of the airspace and the type of value (MSL, GND, FL); lower and
     * lowerType give the value for the lower limit.
     */
    Airspace(QString n, BaseMapElement::objectType t, QPolygon pP,
             int upper, BaseMapElement::elevationType upperType,
             int lower, BaseMapElement::elevationType lowerType);

    /**
     * Destructor, does nothing special.
     */
    ~Airspace();

    /**
     * Draws the airspace into the given painter.
     * Return a pointer to the drawn region or 0.
     *
     * @param targetP The painter to draw the element into.
     *
     * @param opacity Sets the opacity of the painter to opacity. The
     * value should be in the range 0.0 to 1.0, where 0.0 is fully
     * transparent and 1.0 is fully opaque.
     */
    void drawRegion( QPainter* targetP, const QRect &viewRect, qreal opacity = 0.0 );

    /**
     * Tells the caller, if the airspace is drawable or not
     */
    
    bool isDrawable()
    {
      return ( glConfig->isBorder(typeID) && __isVisible() );
    };

    /**
     * Return a pointer to the mapped airspace region data. The caller takes
     * the ownership about the returned object.
     */
    QRegion* createRegion();

    /**
     * Returns the upper limit of the airspace.
     */
    unsigned int getUpperL() const
    {
        return (int)uLimit.getMeters();
    };

    /**
     * Returns the lower limit of the airspace.
     */
    unsigned int getLowerL() const
    {
        return (int)lLimit.getMeters();
    };

    /**
     * Returns the type of the upper limit (MSN, GND, FL)
     * @see BaseMapElement#elevationType
     * @see #uLimitType
     */
    BaseMapElement::elevationType getUpperT() const
    {
        return uLimitType;
    };

    /**
     * Returns the type of the lower limit (MSN, GND, FL)
     * @see BaseMapElement#elevationType
     * @see #lLimitType
     */
    BaseMapElement::elevationType getLowerT() const
    {
        return lLimitType;
    };

    /**
     * Returns a html-text-string about the airspace containing the name,
     * the type and the borders.
     * @return the infostring
     */
    QString getInfoString() const;

    /**
     * Returns a text representing the type of the airspace
     */
    static QString getTypeName (objectType);

    /**
     * Returns true if the given altitude conflicts with the airspace properties
     */
    ConflictType conflicts (const AltitudeCollection& alt,
                            const AirspaceWarningDistance& dist) const;

    /**
     * Returns the last vertical conflict type
     */
    ConflictType lastVConflict()
    {
        return _lastVConflict;
    };

    /**
     * sets the touch time of air space to current time
     */
    void setLastNear()
    {
        _lastNear.start();
    };
    void setLastVeryNear()
    {
        _lastVeryNear.start();
    };
    void setLastInside()
    {
        _lastInside.start();
    };

    /**
    * returns the touch time of air space
    */
    QTime& getLastNear()
    {
        return _lastNear;
    };
    QTime& getLastVeryNear()
    {
        return _lastVeryNear;
    };
    QTime& getLastInside()
    {
        return _lastInside;
    };

    /**
     * @returns the associated AirRegion object
     */
    AirRegion* getAirRegion()
    {
        return m_airRegion;
    }

    /**
     * Sets the associated AirRegion object
     */
    void setAirRegion(AirRegion* region)
    {
        m_airRegion = region;
    }

    /**
     * Compares two items, in this case, Airspaces.
     * The items are compared on their levels. Because cumulus provides a view
     * where the user looks down on the map, the first airspace you'll see is the
     * one with the highest ceiling. So, an item with a heigher ceiling is
     * bigger than an item with a lower ceiling. In case these are the same,
     * the item with the bigger floor will be the bigger item.
     *
     * The airspaces in the list are sorted in this way to make sure they are
     * stacked correctly. This has become important with the introduction of
     * transparent airspaces. By sorting the airspaces like this, the lower ones
     * will be drawn first, and the higher ones on top of them.
     */
    bool operator < (const Airspace& other) const;


private:
    /**
     * Contains the lower limit.
     * @see #getLowerL
     */
    Altitude lLimit;
    /**
     * Contains the type of the lower limit
     * @see #lLimit
     * @see #getLowerT
     */
    BaseMapElement::elevationType lLimitType;
    /**
     * Contains the upper limit.
     * @see #getUpperL
     */
    Altitude uLimit;
    /**
     * Contains the type of the upper limit
     * @see #uLimit
     * @see #getUpperT
     */
    BaseMapElement::elevationType uLimitType;

    BaseMapElement::objectType type;

    mutable ConflictType _lastVConflict;

    /** save time of last touch of airspace */
    QTime _lastNear;
    QTime _lastVeryNear;
    QTime _lastInside;

    // pointer to associated airRegion object
    AirRegion* m_airRegion;
};

struct CompareAirspaces
{
  // The operator sorts the airspaces in the expected order
  bool operator()(const Airspace *as1, const Airspace* as2) const
  {
    int a1C = as1->getUpperL(), a2C = as2->getUpperL();

    if (a1C > a2C)
      {
        return false;
      }

    if (a1C < a2C)
      {
        return true;
      }

    // equal
    int a1F = as1->getLowerL();
    int a2F = as2->getLowerL();
    return (a1F < a2F);
  };
};

/**
 * Specialized QList for Airspaces. The sort member function
 * has been re-implemented to make it possible to sort items based on their
 * levels.
 */
class SortableAirspaceList : public QList<Airspace*>
{
public:
  void sort ()
  {
    // @AP: using std::sort because qSort can not handle pointer
    // elements
    std::sort( begin(), end(), CompareAirspaces() );
  };
};



#endif
