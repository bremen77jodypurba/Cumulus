/***********************************************************************
**
**   lineelement.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2000      by Heiner Lamprecht, Florian Ehinger
**                   2008-2014 by Axel Pauli
**
**   This file is distributed under the terms of the General Public
**   License. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#ifndef LINE_ELEMENT_H
#define LINE_ELEMENT_H

#include "basemapelement.h"

/**
 * \class LineElement
 *
 * \author Heiner Lamprecht, Florian Ehinger, Axel Pauli
 *
 * \brief Class used for all elements, which consist of a point array.
 *
 * \see BaseMapElement#objectType
 *
 * \date 2000-2014
 *
 * \version $Id$
 *
 */

class LineElement : public BaseMapElement
{
 public:

   LineElement();

    /**
     * Creates a new line map element.
     *
     * @param  name  The element name.
     * @param  type  The object type, \ref BaseMapElement#objectType.
     * @param  pP    The point array containing the positions
     * @param  isVal True, if the element is a "valley".
     * @param  secID The number of the map segment.
     * @param  country The country of the element as two letter code
     */
    LineElement( const QString& name,
                 const BaseMapElement::objectType type,
                 const QPolygon& pP,
                 const bool isVal = false,
                 const unsigned short secID=0,
                 const QString& country="" );
    /**
     * Destructor.
     */
    virtual ~LineElement();

    /**
     * Draws the element into the given painter. Reimplemented from
     * BaseMapElement.
     *
     * @param  targetP  The painter to draw the element into.
     * @return true, if element was drawn otherwise false.
     */
    virtual bool drawMapElement(QPainter* targetP);

    /**
     * @return "true", if the element is a valley.
     *
     * @see #valley
     */
    virtual bool isValley() const
      {
        return valley;
      };

    /**
     * Proofs, if the object is in the drawing-area of the map.
     *
     * @return "true", if the bounding-box of the element intersects
     *         with the drawing-area of the map.
     */
    virtual bool isVisible() const
      {
        return glMapMatrix->isVisible(bBox, getTypeID());
      };

    /**
     * Returns the bounding box of the line element on the screen.
     */
    QRect getScreenBoundingBox() const
    {
      return sbBox;
    };

    /**
      * Returns the projected positions of the line element.
      */
    const QPolygon& getProjectedPolygon() const
    {
      return projPolygon;
    };

    /**
     * Sets the polygon of the line element containing the projected positions
     * of the line element.
     *
     * \param newPolygon Polygon with projected coordinate points.
     */
    void setProjectedPolygon( const QPolygon& newPolygon )
    {
      projPolygon = newPolygon;
      bBox = newPolygon.boundingRect();
    };

protected:
    /**
     * Contains the projected positions of the line element.
     */
    QPolygon projPolygon;

    /**
     * The bounding-box of the line element.
     */
    QRect bBox;

    /**
     * The bounding box of the line element on the screen.
     */
    QRect sbBox;

    /**
     * "true", if the element is a valley. Valleys are used for Isohypsen
     * and f.e. unurban-areas within urban areas or island in a lake.
     */
    bool valley;

    /**
     * "true", if the element is a closed polygon (like cities).
     */
    bool closed;
};

#endif
