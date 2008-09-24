/***********************************************************************
 **
 **   airport.h
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

#ifndef AIRPORT_H
#define AIRPORT_H

#include <QString>
#include <QHash>
#include <QStringList>

#include "singlepoint.h"
#include "runway.h"

/**
 * @short Class to handle airports.
 *
 * This class is used for handling airports. The object can be one of
 * Airport, MilAirport, CivMilAirport, Airfield, ClosedAirfield,
 * CivHeliport, MilHeliport, AmbHeliport.
 *
 * @see BaseMapElement#objectType
 */

class Airport : public SinglePoint
{
 public:

  /**
   * Used to define the surface of a runway.
   */
  enum SurfaceType {Unknown = 0, Grass = 1, Asphalt = 2, Concrete = 3};

  /**
   * Creates a new Airport-object.
   * @param  name  The name
   * @param  icao  The icao-name
   * @param  shortName  The abbreviation, used for the gps-logger
   * @param  typeID  The typeid
   * @param  pos  The position
   * @param  elevation  The elevation
   * @param  frequency  The frequency
   * @param  vdf  "true",
   */
  Airport( const QString& name,
           const QString& icao,
           const QString& shortName,
           const BaseMapElement::objectType typeId,
           const WGSPoint& wgsPos,
           const QPoint& pos,
           const Runway& rw,
           const unsigned int elevation,
           const QString& frequency );

  /**
   * Destructor
   */
  virtual ~Airport();

  /**
   * @return the frequency of the airport.
   */
  QString getFrequency() const
    {
      return frequency;
    };

  /**
   * @return ICAO name
   */
  QString getICAO() const
    {
      return icao;
    };

  /**
   * @return a runway object, containing the data of the runway.
   */
  const Runway& getRunway()
    {
      return rwData;
    };

  /**
   * Return a short html-info-string about the airport, containing the
   * name, the alias, the elevation and the frequency as well as a small
   * icon of the airport type.
   *
   * Reimplemented from SinglePoint (@ref SinglePoint#getInfoString).
   * @return the info string
   */
  virtual QString getInfoString() const;

  /**
   * Get translation string for surface type.
   */
  static QString item2Text( const int surfaceType, QString defaultValue=QString("") );

  /**
   * Get surface type for translation string.
   */
  static const int text2Item( const QString& text );

  /**
   * Get sorted translations
   */
  static QStringList& getSortedTranslationList();

  // Draws the element into the given painter.
  virtual void drawMapElement(QPainter* targetP);

 private:

   /**
    * The icao name
    */
  QString icao;

   /**
    * The frequency
    */
   QString frequency;

  /**
   * Contains the runway data.
   */
  Runway rwData;

  /**
   * Contains the shift of the runway during drawing.
   */
  unsigned short rwShift;

  /**
   * Static pointer to surface translations
   */
  static QHash<int, QString> surfaceTranslations;
  static QStringList sortedTranslations;

  /**
   * Static method for loading of object translations
   */
  static void loadTranslations();
};

#endif
