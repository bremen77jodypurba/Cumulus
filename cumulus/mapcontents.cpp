/**********************************************************************
 **
 **   mapcontents.cpp
 **
 **   This file is part of Cumulus.
 **
 ************************************************************************
 **
 **   Copyright (c):  2000      by Heiner Lamprecht, Florian Ehinger
 **                   2008-2012 by Axel Pauli
 **
 **   This file is distributed under the terms of the General Public
 **   License. See the file COPYING for more information.
 **
 **   $Id$
 **
 ***********************************************************************/

#include <cmath>
#include <cstdlib>
#include <unistd.h>

#include <QtGui>

#include "airfield.h"
#include "airspace.h"
#include "basemapelement.h"
#include "calculator.h"
#include "datatypes.h"
#include "filetools.h"
#include "flighttask.h"
#include "generalconfig.h"
#include "gpsnmea.h"
#include "hwinfo.h"
#include "isohypse.h"
#include "lineelement.h"
#include "mainwindow.h"
#include "mapcalc.h"
#include "mapcontents.h"
#include "mapmatrix.h"
#include "mapview.h"
#include "openairparser.h"
#include "projectionbase.h"
#include "radiopoint.h"
#include "singlepoint.h"
#include "waypointcatalog.h"
#include "welt2000.h"
#include "wgspoint.h"

extern MapView* _globalMapView;

// number of last map tile, possible range goes 0...16200
#define MAX_TILE_NUMBER 16200

// general KFLOG file token: @KFL
#define KFLOG_FILE_MAGIC    0x404b464c

// uncompiled map file types
#define FILE_TYPE_AERO        0x41
#define FILE_TYPE_GROUND      0x47
#define FILE_TYPE_TERRAIN     0x54
#define FILE_TYPE_MAP         0x4d

// compiled map file types
#define FILE_TYPE_GROUND_C    0x67
#define FILE_TYPE_TERRAIN_C   0x74
#define FILE_TYPE_MAP_C       0x6d
#define FILE_TYPE_AIRSPACE_C  0x61 // used by OpenAir parser
#define FILE_TYPE_AIRFIELD_C  0x62 // used by Welt2000 parser

// versions
#define FILE_FORMAT_ID        100 // used to handle a previous version
#define FILE_VERSION_GROUND   102
#define FILE_VERSION_TERRAIN  102
#define FILE_VERSION_MAP      101

// compiled version
#define FILE_VERSION_GROUND_C   104
#define FILE_VERSION_TERRAIN_C  104
#define FILE_VERSION_MAP_C      103


#define READ_POINT_LIST\
  if (compiling) {\
    in >> locLength;\
    all.resize(locLength);\
    for(uint i = 0; i < locLength; i++) { \
      in >> lat_temp; \
      in >> lon_temp; \
      all.setPoint(i, _globalMapMatrix->wgsToMap(lat_temp, lon_temp)); \
    }\
    ShortSave(out, all);\
  } else\
    ShortLoad(in, all);\

// Minimum amount of required free memory to start loading of a map file.
// Do not under run this limit, OS can freeze is such a case.
#define MINIMUM_FREE_MEMORY 1024*25

// List of used elevation levels in meters (51 in total):
const short MapContents::isoLevels[] =
{
  -10, 0, 10, 25, 50, 75, 100, 150, 200, 250,
  300, 350, 400, 450, 500, 600, 700, 800, 900, 1000, 1250, 1500, 1750,
  2000, 2250, 2500, 2750, 3000, 3250, 3500, 3750, 4000, 4250, 4500,
  4750, 5000, 5250, 5500, 5750, 6000, 6250, 6500, 6750, 7000, 7250,
  7500, 7750, 8000, 8250, 8500, 8750
};

MapContents::MapContents(QObject* parent, WaitScreen* waitscreen) :
    QObject(parent),
    isFirst(true),
    isReload(false)
#ifdef INTERNET

    , downloadManger(0),
    shallDownloadData(false),
    hasAskForDownload(false)

#endif
{
  ws = waitscreen;

  // Setup a hash used as reverse mapping from isoLine elevation value
  // to color array index.
  for ( uchar i = 0; i < ISO_LINE_LEVELS; i++ )
    {
      isoHash.insert( isoLevels[i], i );
    }

  _nextIsoLevel=10000;
  _lastIsoLevel=-1;
  _isoLevelReset=true;
  _lastIsoEntry=0;

  // read in waypoint list from catalog
  WaypointCatalog wpCat;
  int ok;
  const char* format;
  QString error;

  if( GeneralConfig::instance()->getWaypointFileFormat() == GeneralConfig::Binary )
    {
      ok = wpCat.readBinary( "", &wpList );
      format = "binary";
    }
  else
    {
      ok = wpCat.readXml( "", &wpList, error );
      format = "xml";
    }

  if( ok )
    {
      qDebug() << "MapContents():" << wpList.size() << "waypoints read from"
               << format << "catalog.";
    }

  currentTask = 0;

  connect( this, SIGNAL(progress(int)), ws, SLOT(slot_Progress(int)) );

  connect( this, SIGNAL(loadingFile(const QString&)),
           ws, SLOT(slot_SetText2(const QString&)) );

  // qDebug("MapContents initialized");
}

MapContents::~MapContents()
{
  if ( currentTask )
    {
      delete currentTask;
    }

  qDeleteAll(airspaceList);
}

// save the current waypoint list
void MapContents::saveWaypointList()
{
  WaypointCatalog wpCat;

  if( GeneralConfig::instance()->getWaypointFileFormat() == GeneralConfig::Binary )
    {
      wpCat.writeBinary( "", wpList );
    }
  else
    {
      wpCat.writeXml( "", wpList );
    }
}

/**
 * This method reads in the ground and terrain files from the original
 * kflog source or from the own compiled source. Compiled sources are
 * created from the original kflog source to have a faster access to the
 * single data items. If the map projection is changed the compiled source
 * must be renewed.
 *
 * Ground files describe the surface at level 0m. They are always read in.
 * Terrain files describe the surface above level 0m. If isoline drawing
 * is switched off, terrain files are never read in.
 *
 * Thanks to Josua Dietze for his contribution of precomputed map files.
 *
 */
bool MapContents::__readTerrainFile( const int fileSecID,
                                     const int fileTypeID )
{
  extern const MapMatrix* _globalMapMatrix;
  bool kflExists, kfcExists;
  bool compiling = false;

  if ( fileTypeID != FILE_TYPE_TERRAIN && fileTypeID != FILE_TYPE_GROUND )
    {
      qWarning( "Requested terrain file type 0x%X is unsupported!", fileTypeID );
      return false;
    }

  // First check if we need to load terrain files.
  if ( fileTypeID == FILE_TYPE_TERRAIN &&
       ( !GeneralConfig::instance()->getMapLoadIsoLines() ))
    {
      // loading of terrain files is switched off by the user
      return true;
    }

  if (memoryFull) //if we already know the memory if full and can't be emptied at this point, just return.
    {
      _globalMapView->message(tr("Out of memory! Map not loaded."));
      return false;
    }

  // check free memory
  int memFree = HwInfo::instance()->getFreeMemory();

  if ( memFree < MINIMUM_FREE_MEMORY )
    {
      if ( !unloadDone)
        {
          unloadMaps();  // try freeing some memory
          memFree = HwInfo::instance()->getFreeMemory();  //re-asses free memory
          if ( memFree < MINIMUM_FREE_MEMORY )
            {
              memoryFull=true; //set flag to indicate that we need not try loading any more mapfiles now.
              qWarning("Cumulus couldn't load file, low on memory! Memory needed: %d kB, free: %d kB", MINIMUM_FREE_MEMORY, memFree );
              _globalMapView->message(tr("Out of memory! Map not loaded."));
              return false;
            }
        }
      else
        {
          memoryFull=true; //set flag to indicate that we need not try loading any more mapfiles now.
          qWarning("Cumulus couldn't load file, low on memory! Memory needed: %d kB, free: %d kB", MINIMUM_FREE_MEMORY, memFree );
          _globalMapView->message(tr("Out of memory! Map not loaded."));
          return false;
        }
    }

  QString kflPathName, kfcPathName, pathName;
  QString kflName, kfcName;

  kflName.sprintf("%c_%.5d.kfl", fileTypeID, fileSecID);
  kflExists = locateFile("landscape/" + kflName, kflPathName);

  kfcName.sprintf("landscape/%c_%.5d.kfc", fileTypeID, fileSecID);
  kfcExists = locateFile(kfcName, kfcPathName);

  if ( ! (kflExists || kfcExists) )
    {
      QString path = GeneralConfig::instance()->getMapRootDir() + "/landscape";

      bool res = false;

#ifdef INTERNET

      res = __askUserForDownload();

      if( res == true )
        {
          res = __downloadMapFile( kflName, path );
        }

#endif

      if( res == false  )
        {
          qWarning( "Cumulus: no map files (%s or %s) found! Please install %s.",
                    kflName.toLatin1().data(), kfcName.toLatin1().data(),
                    kflName.toLatin1().data() );
        }

      return false; // file could not be located in any of the possible map directories.
    }

  if ( kflExists )
    {
      if ( kfcExists )
        {
          // kfl file newer than kfc ? Then compile it
          if (getDateFromMapFile( kflPathName ) > getDateFromMapFile( kfcPathName ))
            {
              compiling = true;
              qDebug("Map file %s has a newer date! Recompiling it from source.",
                     kflPathName.toLatin1().data() );
            }
        }
      else
        {
          // no kfc file, we compile anyway
          compiling = true;
        }
    }

  // what file do we read after all ?
  if ( compiling )
    {
      pathName = kflPathName;
      kfcPathName = kflPathName;
      kfcPathName.replace( kfcPathName.length()-1, 1, QString("c") );
    }
  else
    {
      pathName = kfcPathName;
      kflPathName = kfcPathName;
      kflPathName.replace( kflPathName.length()-1, 1, QString("l") );
    }

  QFile mapfile(pathName);

  if ( !mapfile.open(QIODevice::ReadOnly) )
    {
      qWarning("Cumulus: Can't open map file %s for reading", pathName.toLatin1().data() );

      if ( ! compiling && kflExists )
        {
          qDebug("Try to use file %s", kflPathName.toLatin1().data());
          // try to remove unopenable file, not sure if this works.
          unlink( pathName.toLatin1().data() );
          return __readTerrainFile( fileSecID, fileTypeID );
        }

      return false;
    }

  emit loadingFile(pathName);

  QDataStream in(&mapfile);

  if( compiling )
    {
      in.setVersion(QDataStream::Qt_3_3);
    }
  else
    {
      in.setVersion(QDataStream::Qt_4_7);
    }

  // qDebug("reading file %s", pathName.toLatin1().data());

  qint8 loadTypeID;
  quint16 loadSecID, formatID;
  quint32 magic;
  QDateTime createDateTime;
  ProjectionBase *projectionFromFile = 0;

  in >> magic;
  in >> loadTypeID;
  in >> formatID;
  in >> loadSecID;
  in >> createDateTime;

  if ( magic != KFLOG_FILE_MAGIC )
    {
      if ( ! compiling && kflExists )
        {
          qWarning("Cumulus: wrong magic key %x read!\n Retry to compile %s.",
                   magic, kflPathName.toLatin1().data());
          mapfile.close();
          unlink( pathName.toLatin1().data() );
          return __readTerrainFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: wrong magic key %x read! Aborting ...", magic);
      return false;
    }

  if (loadTypeID != fileTypeID) // wrong type
    {
      mapfile.close();

      if ( ! compiling && kflExists )
        {
          qWarning("Cumulus: wrong load type identifier %x read! "
                   "Retry to compile %s",
                   loadTypeID, kflPathName.toLatin1().data() );
          unlink( pathName.toLatin1().data() );
          return __readTerrainFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: %s wrong load type identifier %x read! Aborting ...",
                pathName.toLatin1().data(), loadTypeID );
      return false;
    }

  // Determine, which file format id is expected
  int expFormatID, expComFormatID;

  if ( fileTypeID == FILE_TYPE_TERRAIN )
    {
      expFormatID = FILE_VERSION_TERRAIN;
      expComFormatID = FILE_VERSION_TERRAIN_C;
    }
  else
    {
      expFormatID = FILE_VERSION_GROUND;
      expComFormatID = FILE_VERSION_GROUND_C;
    }

  QFileInfo fi( pathName );

  qDebug("Reading File=%s, Magic=0x%x, TypeId=%c, formatId=%d, Date=%s",
         fi.fileName().toLatin1().data(), magic, loadTypeID, formatID,
         createDateTime.toString(Qt::ISODate).toLatin1().data() );

  if ( compiling )
    {
      // Check map file
      if ( formatID < expFormatID )
        {
          // too old ...
          qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                   "Aborting ...", formatID, expFormatID );
          return false;
        }
      else if (formatID > expFormatID )
        {
          // too new ...
          qWarning("Cumulus: File format too new! (version %d, expecting: %d) "
                   "Aborting ...", formatID,expFormatID );
          return false;
        }
    }
  else
    {
      // Check compiled file
      if ( formatID < expComFormatID )
        {
          // too old ...
          if ( kflExists )
            {
              qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                       "Retry to compile %s",
                       formatID, expComFormatID, kflPathName.toLatin1().data() );
              mapfile.close();
              unlink( pathName.toLatin1().data() );
              return __readTerrainFile( fileSecID, fileTypeID );
            }

          qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                   "Aborting ...", formatID, expFormatID );
          return false;
        }
      else if (formatID > expComFormatID )
        {
          // too new ...
          if ( kflExists )
            {
              qWarning( "Cumulus: File format too new! (version %d, expecting: %d) "
                        "Retry to compile %s",
                        formatID, expComFormatID, kflPathName.toLatin1().data() );
              mapfile.close();
              unlink( pathName.toLatin1().data() );
              return __readTerrainFile( fileSecID, fileTypeID );
            }

          qWarning("Cumulus: File format too new! (version %d, expecting: %d) "
                   "Aborting ...", formatID, expFormatID );
          return false;
        }
    }

  if ( loadSecID != fileSecID )
    {
      if ( ! compiling && kflExists )
        {
          qWarning( "Cumulus: %s: wrong section, bogus file name!"
                    "\n Retry to compile %s",
                    pathName.toLatin1().data(), kflPathName.toLatin1().data() );
          mapfile.close();
          unlink( pathName.toLatin1().data() );
          return __readTerrainFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: %s: wrong section, bogus file name! Arborting ...",
               pathName.toLatin1().data() );
      return false;
    }

  if ( ! compiling )
    {
      // check projection parameters from file against current used values
      projectionFromFile = LoadProjection(in);
      ProjectionBase *currentProjection = _globalMapMatrix->getProjection();

      if ( ! compareProjections( projectionFromFile, currentProjection ) )
        {
          delete projectionFromFile;
          mapfile.close();

          if ( kflExists )
            {
              qWarning( "Cumulus: %s, can't use file, compiled for another projection!"
                        "\n Retry to compile %s",
                        pathName.toLatin1().data(), kflPathName.toLatin1().data() );

              unlink( pathName.toLatin1().data() );
              return __readTerrainFile( fileSecID, fileTypeID );
            }

          qWarning( "Cumulus: %s, can't use file, compiled for another projection!"
                    " Please install %s file and restart.",
                    pathName.toLatin1().data(), kflPathName.toLatin1().data() );
          return false;
        }
      else
        {
          // Must be deleted after use to avoid memory leak
          delete projectionFromFile;
        }
    }

  // Got to initialize "out" stream properly, even if write file is not needed
  QFile ausgabe(kfcPathName);
  QDataStream out(&ausgabe);

  if( compiling )
    {
      out.setDevice(&ausgabe);
      out.setVersion(QDataStream::Qt_4_7);

      if (!ausgabe.open(QIODevice::WriteOnly))
        {
          mapfile.close();
          qWarning("Cumulus: Can't open compiled map file %s for writing!"
                   " Arborting...",
                   kfcPathName.toLatin1().data() );
          return false;
        }

      qDebug("writing file %s", kfcPathName.toLatin1().data());

      out << magic;
      out << loadTypeID;
      out << quint16(expComFormatID);
      out << loadSecID;

      //set time one second later than the time of the original file;
      out << createDateTime.addSecs(1);

      // save current projection data
      SaveProjection(out, _globalMapMatrix->getProjection() );
    }

  int loop = 0;

  while ( !in.atEnd() )
    {
      qint16 elevation;
      qint32 pointNumber, lat, lon;
      QPolygon isoline;

      in >> elevation;

      if ( compiling )
        {
          in >> pointNumber;
          isoline.resize( pointNumber );

          for (int i = 0; i < pointNumber; i++)
            {
              in >> lat;
              in >> lon;

              // This is what causes the long delays, lots of floating point calculations
              isoline.setPoint( i, _globalMapMatrix->wgsToMap(lat, lon));
            }

          // Check, if first point and last point of the isoline identical. In this
          // case we can remove the last point and repeat the check.
          for( int i = isoline.size() - 1; i >= 0; i-- )
            {
              if( isoline.point(0) == isoline.point(i) )
                 {
                   //qWarning( "Isoline Tile=%d has same start and end point. Remove end point.",
                   //           loadSecID );

                   // remove last point and check again
                   isoline.remove(i);
                   continue;
                 }

              break;
            }

          if( isoline.size() < 3)
            {
              // ignore to small isolines
              qWarning( "Isoline Tile=%d, elevation=%dm has to less points!",
                         loadSecID, elevation );
              continue;
            }

          out << elevation;
          // And that is the whole trick: saving the computed result. Takes the same space!
          ShortSave( out, isoline );
        }
      else
        {
          // Reading the computed result from kfc file
          ShortLoad( in, isoline );
        }

      // determine elevation index, 0 is returned as default for not existing values
      uchar elevationIdx = isoHash.value( elevation, 0 );

      Isohypse newItem(isoline, elevation, elevationIdx, fileSecID, fileTypeID);

      // Check in which map the isohypse has to be stored. We do use two
      // different maps, one for Ground and another for Terrain. The default
      // is set to terrain because there are a lot more.
      QMap<int, QList<Isohypse> > *usedMap = &terrainMap;

      if( fileTypeID == FILE_TYPE_GROUND )
        {
          usedMap = &groundMap;
        }

      // Store new isohypse in the isomap. The tile section identifier is the key.
      if( usedMap->contains( fileSecID ))
        {
          // append isohypse to existing vector
          QList<Isohypse>& isoList = (*usedMap)[fileSecID];
          isoList.append( newItem );
        }
      else
        {
          // create a new entry in the isomap
          QList<Isohypse> isoList;
          isoList.append( newItem );
          usedMap->insert( fileSecID, isoList );
        }

      // qDebug("Isohypse added: Size=%d, Elevation=%d, FileTypeID=%c",
      //       isoline.size(), elevation, fileTypeID );

      // AP: Performance brake! emit progress calls wait screen and
      // this steps into main loop
      if ( compiling && (++loop % 100) == 0 )
        {
          emit progress(2);
        }
    }

  // qDebug("loop=%d", loop);
  mapfile.close();

  if ( compiling )
    {
      ausgabe.close();
    }

  return true;
}

bool MapContents::__readBinaryFile(const int fileSecID,
                                   const char fileTypeID)
{
  extern const MapMatrix * _globalMapMatrix;
  bool kflExists, kfcExists;
  bool compiling = false;

  //first, check if we need to load this file at all...
  if ( fileTypeID == FILE_TYPE_TERRAIN &&
       ( !GeneralConfig::instance()->getMapLoadIsoLines() ))
    {
      return true;
    }

  if (memoryFull)   //if we already know the memory if full and can't be emptied at this point, just return.
    {
      _globalMapView->message(tr("Out of memory! Map not loaded."));
      return false;
    }

  //check free memory
  int memFree = HwInfo::instance()->getFreeMemory();

  if ( memFree < MINIMUM_FREE_MEMORY )
    {
      if ( !unloadDone)
        {
          unloadMaps();  //try freeing some memory
          memFree = HwInfo::instance()->getFreeMemory();  //re-asses free memory
          if ( memFree < MINIMUM_FREE_MEMORY )
            {
              memoryFull=true; //set flag to indicate that we need not try loading any more mapfiles now.
              qWarning("Cumulus couldn't load file, low on memory! Memory needed: %d kB, free: %d kB", MINIMUM_FREE_MEMORY, memFree );
              _globalMapView->message(tr("Out of memory! Map not loaded."));
              return false;
            }
        }
      else
        {
          memoryFull=true; //set flag to indicate that we need not try loading any more mapfiles now.
          qWarning("Cumulus couldn't load file, low on memory! Memory needed: %d kB, free: %d kB", MINIMUM_FREE_MEMORY, memFree );
          _globalMapView->message(tr("Out of memory! Map not loaded."));
          return false;
        }
    }

  QString kflPathName, kfcPathName, pathName;
  QString kflName, kfcName;

  kflName.sprintf("%c_%.5d.kfl", fileTypeID, fileSecID);
  kflExists = locateFile("landscape/" + kflName, kflPathName);

  kfcName.sprintf("landscape/%c_%.5d.kfc", fileTypeID, fileSecID);
  kfcExists = locateFile(kfcName, kfcPathName);

  if ( ! (kflExists || kfcExists) )
    {
      QString path = GeneralConfig::instance()->getMapRootDir() + "/landscape";

      bool res = false;

#ifdef INTERNET

      res = __askUserForDownload();

      if( res == true )
        {
          res = __downloadMapFile( kflName, path );
        }

#endif

      if( res == false  )
        {
          qWarning( "Cumulus: no map files (%s or %s) found! Please install %s.",
                    kflName.toLatin1().data(), kfcName.toLatin1().data(),
                    kflName.toLatin1().data() );
        }

      return false; // file could not be located in any of the possible map directories.
    }

  if ( kflExists )
    {
      if ( kfcExists )
        // kfl file newer than kfc ? Then compile it
        {
          if ( getDateFromMapFile( kflPathName ) > getDateFromMapFile( kfcPathName ) )
            {
              compiling = true;
              qDebug("Map file %s has a newer date! Recompiling it from source.",
                     kflPathName.toLatin1().data() );
            }
        }
      else
        {
          // no kfc file, we compile anyway
          compiling = true;
        }
    }

  // what file do we read after all ?
  if ( compiling )
    {
      pathName = kflPathName;
      kfcPathName = kflPathName;
      kfcPathName.replace( kfcPathName.length()-1, 1, QString("c") );
    }
  else
    {
      pathName = kfcPathName;
      kflPathName = kfcPathName;
      kflPathName.replace( kflPathName.length()-1, 1, QString("l") );
    }

  QFile mapfile(pathName);

  if (!mapfile.open(QIODevice::ReadOnly))
    {
      if ( ! compiling && kflExists )
        {
          qDebug("Cumulus: Can't open map file %s for reading!"
                 " Try to use file %s",
                 pathName.toLatin1().data(), kflPathName.toLatin1().data());
          // try to remove unopenable file, not sure if this works.
          unlink( pathName.toLatin1().data() );
          return __readBinaryFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: Can't open map file %s for reading! Aborting ...",
               pathName.toLatin1().data() );
      return false;
    }

  emit loadingFile(pathName);

  QDataStream in(&mapfile);

  if( compiling )
    {
      in.setVersion( QDataStream::Qt_2_0 );
    }
  else
    {
      in.setVersion( QDataStream::Qt_4_7 );
    }

  // qDebug("reading file %s", pathName.toLatin1().data());

  qint8 loadTypeID;
  quint16 loadSecID, formatID;
  quint32 magic;
  QDateTime createDateTime;
  ProjectionBase *projectionFromFile = 0;

  in >> magic;

  if ( magic != KFLOG_FILE_MAGIC )
    {
      if ( ! compiling && kflExists )
        {
          qWarning("Cumulus: wrong magic key %x read!\n Retry to compile %s.",
                   magic, kflPathName.toLatin1().data());
          mapfile.close();
          unlink( pathName.toLatin1().data() );
          return __readBinaryFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: wrong magic key %x read! Aborting ...", magic);
      mapfile.close();
      return false;
    }

  in >> loadTypeID;

  /** Originally, the binary files were mend to come in different flavors.
   * Now, they are all of type 'm'. Use that fact to do check for the
   * compiled or the uncompiled version. */
  if (compiling)
    {
      // uncompiled maps have a different format identifier than compiled
      // maps
      if (loadTypeID != FILE_TYPE_MAP)
        {
          qWarning("Cumulus: wrong load type identifier %x read! Aborting ...",
                   loadTypeID );
          mapfile.close();
          return false;
        }
    }
  else
    {
      if ( loadTypeID != FILE_TYPE_MAP_C) // wrong type
        {
          mapfile.close();

          if ( kflExists )
            {
              qWarning("Cumulus: wrong load type identifier %x read! "
                       "Retry to compile %s",
                       loadTypeID, kflPathName.toLatin1().data() );
              unlink( pathName.toLatin1().data() );
              return __readBinaryFile( fileSecID, fileTypeID );
            }

          qWarning("Cumulus: wrong load type identifier %x read! Aborting ...",
                   loadTypeID );
          return false;
        }
    }

  // Check the version of the subtype. This can be different for the
  // compiled and the uncompiled version.
  in >> formatID;
  in >> loadSecID;
  in >> createDateTime;

  QFileInfo fi( pathName );

  qDebug("Reading File=%s, Magic=0x%x, TypeId=%c, FormatId=%d, Date=%s",
         fi.fileName().toLatin1().data(), magic, loadTypeID, formatID,
         createDateTime.toString(Qt::ISODate).toLatin1().data() );

  if (compiling)
    {
      if ( formatID < FILE_VERSION_MAP)
        {
          // to old ...
          qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                   "Aborting ...", formatID, FILE_VERSION_MAP );
          mapfile.close();
          return false;
        }
      else if (formatID > FILE_VERSION_MAP)
        {
          // to new ...
          qWarning("Cumulus: File format too new! (version %d, expecting: %d) "
                   "Aborting ...", formatID, FILE_VERSION_MAP );
          mapfile.close();
          return false;
        }
    }
  else
    {
      if ( formatID < FILE_VERSION_MAP_C)
        {
          // to old ...
          mapfile.close();

          if ( kflExists )
            {
              qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                       "Retry to compile %s",
                       formatID, FILE_VERSION_MAP_C, kflPathName.toLatin1().data() );
              unlink( pathName.toLatin1().data() );
              return __readBinaryFile( fileSecID, fileTypeID );
            }

          qWarning("Cumulus: File format too old! (version %d, expecting: %d) "
                   "Aborting ...", formatID, FILE_VERSION_MAP_C );
          return false;
        }
      else if (formatID > FILE_VERSION_MAP_C)
        {
          // to new ...
          mapfile.close();

          if ( kflExists )
            {
              qWarning( "Cumulus: File format too new! (version %d, expecting: %d) "
                        "Retry to compile %s",
                        formatID, FILE_VERSION_MAP_C, kflPathName.toLatin1().data() );
              unlink( pathName.toLatin1().data() );
              return __readBinaryFile( fileSecID, fileTypeID );
            }

          qWarning("Cumulus: File format too new! (version %d, expecting: %d) "
                   "Aborting ...", formatID, FILE_VERSION_MAP_C );

          return false;
        }
    }

  // check if this section really covers the area we want to deal with
  if ( loadSecID != fileSecID )
    {
      mapfile.close();

      if ( ! compiling && kflExists )
        {
          qWarning( "Cumulus: %s: wrong section, bogus file name!"
                    "\n Retry to compile %s",
                    pathName.toLatin1().data(), kflPathName.toLatin1().data() );
          unlink( pathName.toLatin1().data() );
          return __readBinaryFile( fileSecID, fileTypeID );
        }

      qWarning("Cumulus: %s: wrong section, bogus file name! Aborting ...",
               pathName.toLatin1().data() );
      return false;
    }

  if ( ! compiling )
    {
      // check projection parameters from file against current used values
      projectionFromFile = LoadProjection(in);
      ProjectionBase *currentProjection = _globalMapMatrix->getProjection();

      if ( ! compareProjections( projectionFromFile, currentProjection ) )
        {
          delete projectionFromFile;
          mapfile.close();

          if ( kflExists )
            {
              qWarning( "Cumulus: %s, can't use file, compiled for another projection!"
                        "\n Retry to compile %s",
                        pathName.toLatin1().data(), kflPathName.toLatin1().data() );

              unlink( pathName.toLatin1().data() );
              return __readBinaryFile( fileSecID, fileTypeID );
            }

          qWarning( "Cumulus: %s, can't use file, compiled for another projection!"
                    " Please install %s file and restart.",
                    pathName.toLatin1().data(), kflPathName.toLatin1().data() );
          return false;
        }
      else
        {
          // Must be deleted after use to avoid memory leak
          delete projectionFromFile;
        }
    }

  QFile ausgabe(kfcPathName);
  QDataStream out(&ausgabe);

  if ( compiling )
    {
      out.setDevice( &ausgabe );
      out.setVersion( QDataStream::Qt_4_7 );

      if (!ausgabe.open(QIODevice::WriteOnly))
        {
          qWarning("Cumulus: Can't open compiled map file %s for writing!"
                   " Aborting ...",
                   kfcPathName.toLatin1().data() );
          mapfile.close();
          return false;
        }

      qDebug("writing file %s", kfcPathName.toLatin1().data());

      out << magic;
      loadTypeID = FILE_TYPE_MAP_C;
      formatID   = FILE_VERSION_MAP_C;
      out << loadTypeID;
      out << formatID;
      out << loadSecID;
      out << createDateTime.addSecs(1);   //set time one second later than the time of the original file;
      SaveProjection(out, _globalMapMatrix->getProjection());
    }

  quint8 lm_typ;
  qint8 sort, elev;
  qint32 lat_temp, lon_temp;
  quint32 locLength = 0;
  QString name = "";

  unsigned int gesamt_elemente = 0;
  uint loop = 0;

  while ( ! in.atEnd() )
    {
      BaseMapElement::objectType typeIn = BaseMapElement::NotSelected;
      in >> (quint8&)typeIn;

      if ( compiling )
        out << (quint8&)typeIn;

      locLength = 0;
      name = "";

      QPolygon all;
      QPoint single;

      gesamt_elemente++;

      switch (typeIn)
        {
        case BaseMapElement::Motorway:
          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadMotorways() ) break;

          motorwayList.append( LineElement("", typeIn, all, false, fileSecID) );
          break;

        case BaseMapElement::Road:
        case BaseMapElement::Trail:
          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadRoads() ) break;

          roadList.append( LineElement("", typeIn, all, false, fileSecID) );
          break;

        case BaseMapElement::Aerial_Cable:
        case BaseMapElement::Railway:
        case BaseMapElement::Railway_D:
          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadRailways() ) break;

          railList.append( LineElement("", typeIn, all, false, fileSecID) );
          break;

        case BaseMapElement::Canal:
        case BaseMapElement::River:
        case BaseMapElement::River_T:

          typeIn = BaseMapElement::River; //don't use different river types internally

          if (formatID >= FILE_FORMAT_ID)
            {
              if ( compiling )
                {
                  in >> name;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }

          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadWaterways() ) break;

          hydroList.append( LineElement(name, typeIn, all, false, fileSecID) );
          break;

        case BaseMapElement::City:
          in >> sort;
          if ( compiling )
            out << sort;

          if (formatID >= FILE_FORMAT_ID)
            {
              if ( compiling )
                {
                  in >> name;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }

          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadCities() ) break;

          cityList.append( LineElement(name, typeIn, all, sort, fileSecID) );
          // qDebug("added city '%s'", name.toLatin1().data());
          break;

        case BaseMapElement::Lake:
        case BaseMapElement::Lake_T:

          typeIn=BaseMapElement::Lake; // don't use different lake type internally
          in >> sort;

          if ( compiling )
            out << sort;

          if (formatID >= FILE_FORMAT_ID)
            {
              if ( compiling )
                {
                  in >> name;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }

          READ_POINT_LIST

          lakeList.append(LineElement(name, typeIn, all, sort, fileSecID));
          // qDebug("appended lake, name='%s', pointCount=%d", name.toLatin1().data(), all.count());
          break;

        case BaseMapElement::Forest:
        case BaseMapElement::Glacier:
        case BaseMapElement::PackIce:
          in >> sort;
          if ( compiling )
            out << sort;

          if (formatID >= FILE_FORMAT_ID)
            {
              if ( compiling )
                {
                  in >> name;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }

          READ_POINT_LIST

          if ( !GeneralConfig::instance()->getMapLoadForests() ||
               typeIn == BaseMapElement::Glacier ||
               typeIn == BaseMapElement::PackIce )
            {
              // Cumulus ignores Glacier and PackIce items
              break;
            }

          topoList.append( LineElement(name, typeIn, all, sort, fileSecID) );
          break;

        case BaseMapElement::Village:

          if (formatID >= FILE_FORMAT_ID)
            {
              if ( compiling )
                {
                  in >> name;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }
          in >> lat_temp;
          in >> lon_temp;

          if ( !GeneralConfig::instance()->getMapLoadCities() ) break;

          if ( compiling )
            {
              single = _globalMapMatrix->wgsToMap(lat_temp, lon_temp);
              out << single;
            }
          else
            {
              in >> single;
            }

          villageList.append( SinglePoint( name,
                                           "",
                                           typeIn,
                                           WGSPoint(lat_temp, lon_temp),
                                           single,
                                           0,
                                           "",
                                           "",
                                           fileSecID ) );
          // qDebug("added village '%s'", name.toLatin1().data());
          break;

        case BaseMapElement::Spot:

          if (formatID >= FILE_FORMAT_ID)
            {
              in >> elev;
              if ( compiling )
                out << elev;
            }

          in >> lat_temp;
          in >> lon_temp;

          if ( !GeneralConfig::instance()->getMapLoadCities() ) break;

          if ( compiling )
            {
              single = _globalMapMatrix->wgsToMap(lat_temp, lon_temp);
              out << single;
            }
          else
            {
              in >> single;
            }

          obstacleList.append( SinglePoint( "Spot",
                                            "",
                                            typeIn,
                                            WGSPoint(lat_temp, lon_temp),
                                            single,
                                            0,
                                            "",
                                            "",
                                            fileSecID ) );
          break;

        case BaseMapElement::Landmark:

          if (formatID >= FILE_FORMAT_ID)
            {
              in >> lm_typ;
              if ( compiling )
                {
                  in >> name;
                  out << lm_typ;
                  ShortSave(out, name);
                }
              else
                {
                  ShortLoad(in, name);
                }
            }

          in >> lat_temp;
          in >> lon_temp;

          if ( !GeneralConfig::instance()->getMapLoadCities() ) break;

          if ( compiling )
            {
              single = _globalMapMatrix->wgsToMap(lat_temp, lon_temp);
              out << single;
            }
          else
            {
              in >> single;
            }

          landmarkList.append( SinglePoint( name,
                               "",
                               typeIn,
                               WGSPoint(lat_temp, lon_temp),
                               single,
                               0,
                               "",
                               "",
                               fileSecID ) );

          // qDebug("added landmark '%s'", name.toLatin1().data());
          break;
        default:
          qWarning ("MapContents::__readBinaryFile; type not handled in switch: %d", typeIn);
          break;
        }

      // @AP: Performance brake! emit progress calls waitscreen and
      // this steps into main loop
      if ( compiling && (++loop % 100) == 0 )
        {
          emit progress(2);
        }
    }

  // qDebug("loop=%d", loop);
  mapfile.close();

  if ( compiling )
    {
      ausgabe.close();
    }

  return true;
}

#ifdef INTERNET

/**
 * Downloads all map tiles enclosed by the square with the center point. The
 * square edges are in parallel with the sky directions N, S, W, E. Inside
 * the square you can place a circle with radius length.
 *
 * @param center The center coordinates (Lat/lon) in KFLog format
 * @param length The half length of the square edge in meters.
 */
void MapContents::slotDownloadMapArea( const QPoint &center, const Distance& length )
{
  double radius = length.getMeters();

  if( radius == 0.0 )
    {
      return;
    }

  // Convert center point from KFLog degree into decimal degree
  double centerLat = center.x() / 600000.;
  double centerLon = center.y() / 600000.;

  // Latitude 90N or 90S causes division by zero. Set it too a smaller value.
  if( centerLat >=90. )
    {
      centerLat = 88.;
    }
  else if( centerLat <=-90. )
    {
      centerLat = -88.;
    }

  // Calculate length in degree along the latitude and the longitude.
  // For the calculation the circle formula  is used.
  double deltaLat = 180/M_PI * radius/RADIUS;
  double deltaLon = 180/M_PI * radius/(RADIUS * cos ( M_PI / 180.0 * centerLat ));

  // Calculate the sky boarders of the square.
  int north = (int) ceil(centerLat + deltaLat);
  int south = (int) floor( centerLat - deltaLat);
  int east  = (int) ceil(centerLon + deltaLon);
  int west  = (int) floor(centerLon - deltaLon);

  // Round up and down to even numbers
  north += abs(north % 2);
  south -= abs(south % 2);

  east += abs(east % 2);
  west -= abs(west % 2);

  // Check and correct boarders to their limits.
  if( north > 90 )
    {
      north = 90;
    }

  if( south < -90 )
    {
      south = -90;
    }

  if( east > 180 )
    {
      east = 180;
    }

  if( west < -180 )
    {
      west = -180;
    }

  qDebug("MapAreaDownloadBox: N=%d, S=%d, E=%d, W=%d", north, south, east, west );

  QString mapDir = GeneralConfig::instance()->getMapRootDir() + "/landscape";

  int needed = 0;

  for( int i = west; i < east; i+=2 )
    {
      for( int j = north; j > south; j-=2 )
        {
          int tile = mapTileNumber( j, i );
          // qDebug("Lat=%d, Lon=%d, Tile=%d", j, i, tile );

          const char fileType[3] = { FILE_TYPE_GROUND,
                                     FILE_TYPE_TERRAIN,
                                     FILE_TYPE_MAP };

          for( uint k = 0; k < sizeof(fileType); k++ )
            {
              QString kflName, kflPathName;

              kflName.sprintf( "%c_%.5d.kfl", fileType[k], tile );

              if( locateFile( "landscape/" + kflName, kflPathName ) == true )
                {
                  // File already exists
                  // qDebug() << "Existing File:" << kflName;
                  continue;
                }

              // File is missing, request it to download
              // qDebug() << "Download File:" << kflName;
              __downloadMapFile( kflName, mapDir );
              needed++;
            }
        }
    }

  qDebug( "MapAreaDownload: %d Maps required by download", needed );
}

/**
 * Try to download a missing ground/terrain/map file.
 *
 * @param file The name of the file without any path prefixes.
 * @param directory The destination directory.
 *
 */
bool MapContents::__downloadMapFile( QString &file, QString &directory )
{
  extern Calculator* calculator;

  if( GpsNmea::gps->getConnected() && calculator->moving() )
    {
      // We have a GPS connection and are in move. That does not allow
      // to make any downloads.
      return false;
    }

  if( downloadManger == static_cast<DownloadManager *> (0) )
    {
      downloadManger = new DownloadManager(this);

      connect( downloadManger, SIGNAL(finished( int, int )),
               this, SLOT(slotDownloadsFinished( int, int )) );

      connect( downloadManger, SIGNAL(networkError()),
               this, SLOT(slotNetworkError()) );

      connect( downloadManger, SIGNAL(status( const QString& )),
               _globalMapView, SLOT(slot_info( const QString& )) );
    }

  QString url = GeneralConfig::instance()->getMapServerUrl() + file;
  QString dest = directory + "/" + file;

  downloadManger->downloadRequest( url, dest );
  return true;
}

/**
 * This slot is called to download the Welt2000 file from the internet.
 * @param welt2000FileName The Welt2000 filename as written at the web page
 * without any path prefixes.
 */
void MapContents::slotDownloadWelt2000( const QString& welt2000FileName )
{
  extern Calculator* calculator;

  if( GpsNmea::gps->getConnected() && calculator->moving() )
    {
      // We have a GPS connection and are in move. That does not allow
      // to make any downloads.
      return;
    }

  if( downloadManger == static_cast<DownloadManager *> (0) )
    {
      downloadManger = new DownloadManager(this);

      connect( downloadManger, SIGNAL(finished( int, int )),
               this, SLOT(slotDownloadsFinished( int, int )) );

      connect( downloadManger, SIGNAL(networkError()),
               this, SLOT(slotNetworkError()) );

      connect( downloadManger, SIGNAL(status( const QString& )),
               _globalMapView, SLOT(slot_info( const QString& )) );
    }

  connect( downloadManger, SIGNAL(welt2000Downloaded()),
           this, SLOT(slotReloadWelt2000Data()) );

  QString url  = GeneralConfig::instance()->getWelt2000Link() + "/" + welt2000FileName;
  QString dest = GeneralConfig::instance()->getMapRootDir() + "/airfields/welt2000.txt";

  downloadManger->downloadRequest( url, dest );
}

/**
 * This slot is called to download an airspace file from the internet.
 * @param url The url of the web page where the file is to find.
 */
void MapContents::slotDownloadAirspace( QString& url )
{
  extern Calculator* calculator;

  if( GpsNmea::gps->getConnected() && calculator->moving() )
    {
      // We have a GPS connection and are in move. That does not allow
      // to make any downloads.
      return;
    }

  if( downloadManger == static_cast<DownloadManager *> (0) )
    {
      downloadManger = new DownloadManager(this);

      connect( downloadManger, SIGNAL(finished( int, int )),
               this, SLOT(slotDownloadsFinished( int, int )) );

      connect( downloadManger, SIGNAL(networkError()),
               this, SLOT(slotNetworkError()) );

      connect( downloadManger, SIGNAL(status( const QString& )),
               _globalMapView, SLOT(slot_info( const QString& )) );
    }

  connect( downloadManger, SIGNAL(airspaceDownloaded() ),
           this, SLOT(slotReloadAirspaceData()) );

  QUrl airspaceUrl( url );
  QString file = QFileInfo( airspaceUrl.path() ).fileName();
  QString dest = GeneralConfig::instance()->getMapRootDir() + "/airspaces/" + file;

  downloadManger->downloadRequest( url, dest );
}

/** Called, if all downloads are finished. */
void MapContents::slotDownloadsFinished( int requests, int errors )
{
  // All has finished, free not more needed resources
  downloadManger->deleteLater();
  downloadManger = static_cast<DownloadManager *> (0);

  // initiate a map redraw
  Map::instance->scheduleRedraw( Map::baseLayer );

  QString msg = QString(tr("%1 download(s) with %2 error(s) done.")).arg(requests).arg(errors);

  QMessageBox mb( QMessageBox::Information,
                  tr("Downloads finished"),
                  msg,
                  QMessageBox::Ok,
                  MainWindow::mainWindow() );

#ifdef ANDROID

  mb.show();
  QPoint pos = MainWindow::mainWindow()->mapToGlobal(QPoint( MainWindow::mainWindow()->width()/2  - mb.width()/2,
                                                             MainWindow::mainWindow()->height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif

  mb.exec();
}

/** Called, if a network error occurred during the downloads. */
void MapContents::slotNetworkError()
{
  // A network error has occurred. We do stop all further downloads.
  downloadManger->deleteLater();
  downloadManger = static_cast<DownloadManager *> (0);

  // Reset user decision flag
  shallDownloadData = false;

  QString msg = QString(tr("Network error occurred.\nAll downloads are canceled!"));

  QMessageBox mb( QMessageBox::Warning,
                  tr("Network Error"),
                  msg,
                  QMessageBox::Ok,
                  MainWindow::mainWindow() );

#ifdef ANDROID

  mb.show();
  QPoint pos = MainWindow::mainWindow()->mapToGlobal(QPoint( MainWindow::mainWindow()->width()/2  - mb.width()/2,
                                                             MainWindow::mainWindow()->height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif

  mb.exec();
}

/**
 * Ask the user once for download of missing Welt200 or map files. The answer
 * is stored permanently to have it for further request.
 * Returns true, if download is desired otherwise false.
 */
bool MapContents::__askUserForDownload()
{
  if( hasAskForDownload == true )
    {
      return shallDownloadData;
    }

  hasAskForDownload = true;

  QMessageBox mb( QMessageBox::Question,
                  tr( "Download missing Data?" ),
                  tr( "Download missing Data from the Internet?" ) +
                  QString("<p>") + tr("Active Internet connection is needed!"),
                  QMessageBox::Yes | QMessageBox::No,
                  MainWindow::mainWindow() );

  mb.setDefaultButton( QMessageBox::No );

#ifdef ANDROID

  mb.show();
  QPoint pos = MainWindow::mainWindow()->mapToGlobal(QPoint( MainWindow::mainWindow()->width()/2  - mb.width()/2,
                                                             MainWindow::mainWindow()->height()/2 - mb.height()/2 ));
  mb.move( pos );

#endif

  if( mb.exec() == QMessageBox::Yes )
    {
      shallDownloadData = true;
    }
  else
    {
      shallDownloadData = false;
    }

  return shallDownloadData;
}

#endif

void MapContents::proofeSection()
{
  // qDebug("MapContents::proofeSection()");

  // @AP: defined a static mutex variable, to prevent the recursive
  // calling of this method
  static bool mutex = false;

  if( mutex )
    {
      // qDebug("MapContents::proofeSection(): is recursive called, returning");
      return; // return immediately, if reenter in method is not possible
    }

  mutex = true;

  extern MapMatrix* _globalMapMatrix;

  // Get map borders in KFLog coordinates. X=Longitude, Y=Latitude.
  QRect mapBorder = _globalMapMatrix->getViewBorder();

  int westCorner = ( ( mapBorder.left() / 600000 / 2 ) * 2 + 180 ) / 2;
  int eastCorner = ( ( mapBorder.right() / 600000 / 2 ) * 2 + 180 ) / 2;
  int northCorner = ( ( mapBorder.top() / 600000 / 2 ) * 2 - 88 ) / -2;
  int southCorner = ( ( mapBorder.bottom() / 600000 / 2 ) * 2 - 88 ) / -2;

  if (mapBorder.left() < 0)
    westCorner -= 1;
  if (mapBorder.right() < 0)
    eastCorner -= 1;
  if (mapBorder.top() < 0)
    northCorner += 1;
  if (mapBorder.bottom() < 0)
    southCorner += 1;

  // qDebug( "MapBorderCorners: l=%d, r=%d, t=%d, b=%d",
  //          westCorner, eastCorner, northCorner, southCorner );

  unloadDone = false;
  memoryFull = false;
  char step, hasstep; // used as small integers
  TilePartMap::Iterator it;

  if( isReload )
    {
      ws->setScreenUsage( true );
      ws->setVisible( true );
      QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents |
                                       QEventLoop::ExcludeSocketNotifiers );
    }

  if( isFirst )
    {
      ws->slot_SetText1( tr( "Loading maps..." ) );
    }

  for( int row = northCorner; row <= southCorner; row++ )
    {
      for( int col = westCorner; col <= eastCorner; col++ )
        {
          int secID = row + (col + (row * 179));
          // qDebug( "Needed BoxSecID=%d", secID );

          if( isFirst )
            {
              // Animate a little bit during first load. Later on in flight,
              // we need the time for GPS processing.
              emit progress( 2 );
            }

          if( secID >= 0 && secID <= MAX_TILE_NUMBER )
            {
              // a valid tile (2x2 degree area) must be in the range 0 ... 16200
              if( ! tileSectionSet.contains( secID ) )
                {
                  // qDebug(" Tile %d is missing", secID );
                  // Tile is missing
                  if( ! isFirst)
                    {
                      // @AP: remove of all unused maps to get place
                      // in heap. That can be disabled here because
                      // the loading routines will also check the
                      // available memory and call the unloadMaps()
                      // method is necessary. But the disadvantage
                      // is in that case that the freeing needs a
                      // lot of time (several seconds).
                      if( GeneralConfig::instance()->getMapUnload() )
                        {
                          unloadMaps(0);
                        }
                    }

                  // qDebug("Going to load sectionID %d", secID);

                  step = 0;
                  // check to see if parts of this tile has already been loaded before
                  it = tilePartMap.find(secID);

                  if (it == tilePartMap.end())
                    {
                      //not found
                      hasstep = 0;
                    }
                  else
                    {
                      hasstep = it.value();
                    }

                  //try loading the currently unloaded files
                  if (!(hasstep & 1))
                    {
                      if (__readTerrainFile(secID, FILE_TYPE_GROUND))
                        step |= 1;
                    }

                  if (!(hasstep & 2))
                    {
                      if (__readTerrainFile(secID, FILE_TYPE_TERRAIN))
                        step |= 2;
                    }

                  if (!(hasstep & 4))
                    {
                      if (__readBinaryFile(secID, FILE_TYPE_MAP))
                        step |= 4;
                    }

                  if (step == 7) //set the correct flags for this map tile
                    {
                      tileSectionSet.insert(secID);  // add section id to set
                      tilePartMap.remove(secID); // make sure we don't leave it as partly loaded
                    }
                  else
                    {
                      if (step > 0)
                        {
                          tilePartMap.insert(secID, step);
                        }
                    }
                }
            }
        }
    }

  if( isFirst )
    {
      ws->slot_SetText2( tr( "Reading OpenAir Files" ) );

      OpenAirParser oap;
      oap.load( airspaceList );

      // finally, sort the airspaces
      airspaceList.sort();

      ws->slot_SetText2( tr( "Reading Welt2000 File" ) );

      if( isReload == false )
        {
          // Load airfield data not in an extra thread
          Welt2000 welt2000;

          if( ! welt2000.load( airfieldList, gliderfieldList, outLandingList ) )
            {

#ifdef INTERNET

              if( __askUserForDownload() == true )
                {
                  // Welt2000 load failed, try to download a new Welt2000 File.
                  slotDownloadWelt2000( GeneralConfig::instance()->getWelt2000FileName() );
                }
#endif
            }
        }
      else
        {

#ifdef WELT2000_THREAD
          // In case of an reload we assume, a Welt2000 file is available.
          // Therefore the reload is done in an extra thread because it can
          // take a while and the GUI shall not be blocked by this action.
          loadWelt2000DataViaThread();
#else
          // Old solution. Should be used, if not enough RAM is available
          // because thread loading needs temporary more memory for parallel load.
          Welt2000 welt2000;
          welt2000.load( airfieldList, gliderfieldList, outLandingList );
#endif
        }

      ws->slot_SetText1(tr("Loading maps done"));
    }

  if( isReload )
    {
      ws->setScreenUsage( false );
      ws->setVisible( false );
    }

  isFirst  = false;
  isReload = false;
  mutex    = false; // unlock mutex
}

// Distance unit is expected as meters. The bounding map rectangle will be
// enlarged by distance.
void MapContents::unloadMaps(unsigned int distance)
{
  // qDebug("MapContents::unloadMaps() is called");

  if( unloadDone )
    {
      return; // we only unload map data once (per map redrawing round)
    }

  extern MapMatrix* _globalMapMatrix;
  QRect mapBorder = _globalMapMatrix->getViewBorder();

  // scale uses unit meter/pixel
  double scale = _globalMapMatrix->getScale(MapMatrix::CurrentScale);

  int width  = (int) rint(scale * distance);
  int height = width;

  //   qDebug("left=%d, right=%d, top=%d, bottom=%d, wight=%d, height=%d, scale=%f",
  //   mapBorder.left(), mapBorder.right(), mapBorder.top(), mapBorder.bottom(),
  //   width, height, scale );

  int westCorner = ( ( ( mapBorder.left() - width ) / 600000 / 2 ) * 2 + 180 ) / 2;
  int eastCorner = ( ( ( mapBorder.right() + width ) / 600000 / 2 ) * 2 + 180 ) / 2;
  int northCorner = ( ( ( mapBorder.top() - height ) / 600000 / 2 ) * 2 - 88 ) / -2;
  int southCorner = ( ( ( mapBorder.bottom() + height ) / 600000 / 2 ) * 2 - 88 ) / -2;

  if (mapBorder.left() < 0)
    westCorner -= 1;
  if (mapBorder.right() < 0)
    eastCorner -= 1;
  if (mapBorder.top() < 0)
    northCorner += 1;
  if (mapBorder.bottom() < 0)
    southCorner += 1;

  QSet<int> currentTileSet; // tiles of the current box

  // Collect all valid tiles of the current box in a set.
  for ( int row = northCorner; row <= southCorner; row++ )
    {
      for (int col = westCorner; col <= eastCorner; col++)
        {
          int secID = row + (col + (row * 179));

          if (secID >= 0 && secID <= MAX_TILE_NUMBER)
            {
              // qDebug( "unloadMaps-active-secId=%d",  secID );
              currentTileSet.insert(secID);
            }
        }
    }

  bool something2free = false;

  // Iterate over all loaded tiles (tileSectionSet) and remove all tiles,
  // which are not contained in the current box.
  foreach( int secID, tileSectionSet )
  {
    if ( !currentTileSet.contains( secID ) )
      {
        // remove not more needed element from related objects
        tilePartMap.remove( secID );
        tileSectionSet.remove( secID );
        something2free = true;
        continue;
      }
  }

  // @AP: check, if something is to free, otherwise we can return to spare
  // processing time
  if ( ! something2free )
    {
      return;
    }

#ifdef DEBUG_UNLOAD_SUM
  // save free memory
  int memFreeBegin = HwInfo::instance()->getFreeMemory();
#endif

  QTime t;
  t.start();

  unloadMapObjects( cityList );

#ifdef DEBUG_UNLOAD
  uint sum = t.elapsed();
  qDebug("Unload cityList(%d), elapsed=%d", cityList.count(), t.restart());
#endif

  unloadMapObjects( hydroList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload hydroList(%d), elapsed=%d", hydroList.count(), t.restart());
#endif

  unloadMapObjects( lakeList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload lakeList(%d), elapsed=%d", lakeList.count(), t.restart());
#endif

  unloadMapObjects( groundMap );
  unloadMapObjects( terrainMap );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload isoList(%d), elapsed=%d", isoList.count(), t.restart());
#endif

  unloadMapObjects( landmarkList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload landmarkList(%d), elapsed=%d", landmarkList.count(), t.restart());
#endif

  unloadMapObjects( radioList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload radioList(%d), elapsed=%d", radioList.count(), t.restart());
#endif

  unloadMapObjects( obstacleList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload obstacleList(%d), elapsed=%d", obstacleList.count(), t.restart());
#endif

  unloadMapObjects( railList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload railList(%d), elapsed=%d", railList.count(), t.restart());
#endif

  unloadMapObjects( reportList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload reportList(%d), elapsed=%d", reportList.count(), t.restart());
#endif

  unloadMapObjects( motorwayList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload motorwayList(%d), elapsed=%d", motorwayList.count(), t.restart());
#endif

  unloadMapObjects( roadList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload roadList(%d), elapsed=%d", roadList.count(), t.restart());
#endif

  unloadMapObjects( topoList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload topoList(%d), elapsed=%d", topoList.count(), t.restart());
#endif

  unloadMapObjects( villageList );

#ifdef DEBUG_UNLOAD
  sum += t.elapsed();
  qDebug("Unload villageList(%d), elapsed=%d", villageList.count(), t.restart());
#endif

  unloadDone=true;

#ifdef DEBUG_UNLOAD_SUM
  // save free memory
  int memFreeEnd = HwInfo::instance()->getFreeMemory();

  // qDebug("Unloaded unneeded map elements. Elapsed Time=%dms, MemStart=%dKB, MemEnd=%dKB, SEDelta=%dKB", sum, memFreeBegin, memFreeEnd, memFreeBegin-memFreeEnd );
#endif

}

/** Updates the projected coordinates of this map object type */
void MapContents::updateProjectedCoordinates( QList<SinglePoint>& list )
{
  extern MapMatrix* _globalMapMatrix;

  for( int i = 0; i < list.count(); i++ )
    {
       QPoint newPos = _globalMapMatrix->wgsToMap( list.at(i).getWGSPosition() );
       list[i].setPosition( newPos );
    }
}

void MapContents::unloadMapObjects( QList<LineElement>& list )
{
  bool renew = false;

  for (int i = list.count() - 1; i >= 0; i--)
    {
       if ( !tileSectionSet.contains(list.at(i).getMapSegment()) )
        {
          list.removeAt(i);
          renew = true;
        }
    }

  if( renew )
    {
      // Setup a new QList to get freed the internal allocated memory.
      // That is a trick and maybe we should use a QLinkedList as alternative.
      QList<LineElement> renew = list;
      list = renew;
    }
}

void MapContents::unloadMapObjects(QList<SinglePoint>& list)
{
  bool renew = false;

  for (int i = list.count() - 1; i >= 0; i--)
    {
      if ( !tileSectionSet.contains(list.at(i).getMapSegment()) )
        {
          list.removeAt(i);
          renew = true;
        }
    }

  if( renew )
    {
      // Setup a new QList to get freed the internal allocated memory.
      // That is a trick and maybe we should use a QLinkedList as alternative.
      QList<SinglePoint> renew = list;
      list = renew;
    }
}

void MapContents::unloadMapObjects(QList<RadioPoint>& list)
{
  for (int i = list.count() - 1; i >= 0; i--)
    {
      if ( !tileSectionSet.contains(list.at(i).getMapSegment()) )
        {
          list.removeAt(i);
        }
    }
}

void MapContents::unloadMapObjects(QMap<int, QList<Isohypse> > isoMap)
{

  QList<int> keys = isoMap.keys();

  for( int i = 0; i < keys.size(); i++ )
   {
     // Tile not in global list, remove it.
     if( ! tileSectionSet.contains(keys.at(i)) )
       {
         isoMap.remove( keys.at(i) );
       }
     }
}

/**
 * clears the content of the given list.
 *
 * @param  listIndex  the index of the list.
 */
void MapContents::clearList(const int listIndex)
{
  switch (listIndex)
    {
    case AirfieldList:
      airfieldList.clear();
      break;
    case GliderfieldList:
      gliderfieldList.clear();
      break;
    case OutLandingList:
      outLandingList.clear();
      break;
    case RadioList:
      radioList.clear();
      break;
    case AirspaceList:
      airspaceList.clear();
      break;
    case ObstacleList:
      obstacleList.clear();
      break;
    case ReportList:
      reportList.clear();
      break;
    case CityList:
      cityList.clear();
      break;
    case VillageList:
      villageList.clear();
      break;
    case LandmarkList:
      landmarkList.clear();
      break;
    case MotorwayList:
      motorwayList.clear();
      break;
    case RoadList:
      roadList.clear();
      break;
    case RailList:
      railList.clear();
      break;
    case HydroList:
      hydroList.clear();
      break;
    case LakeList:
      lakeList.clear();
      break;
    case TopoList:
      topoList.clear();
      break;
    default:
      qWarning( "MapContents::clearList(): unknown list type %d!", listIndex );
      break;
    }
}


unsigned int MapContents::getListLength( const int listIndex ) const
{
  switch (listIndex)
    {
    case AirfieldList:
      return airfieldList.count();
    case GliderfieldList:
      return gliderfieldList.count();
    case OutLandingList:
      return outLandingList.count();
    case RadioList:
      return radioList.count();
    case AirspaceList:
      return airspaceList.count();
    case ObstacleList:
      return obstacleList.count();
    case ReportList:
      return reportList.count();
    case CityList:
      return cityList.count();
    case VillageList:
      return villageList.count();
    case LandmarkList:
      return landmarkList.count();
    case MotorwayList:
      return motorwayList.count();
    case RoadList:
      return roadList.count();
    case RailList:
      return railList.count();
    case HydroList:
      return hydroList.count();
    case LakeList:
      return lakeList.count();
    case TopoList:
      return topoList.count();
    default:
      return 0;
    }
}

Airspace* MapContents::getAirspace(unsigned int index)
{
  return static_cast<Airspace *> (airspaceList[index]);
}

Airfield* MapContents::getAirfield(unsigned int index)
{
  return &airfieldList[index];
}

Airfield* MapContents::getGliderfield(unsigned int index)
{
  return &gliderfieldList[index];
}

Airfield* MapContents::getOutlanding(unsigned int index)
{
  return &outLandingList[index];
}

BaseMapElement* MapContents::getElement(int listType, unsigned int index)
{
  switch (listType)
    {
    case AirfieldList:
      return &airfieldList[index];
    case GliderfieldList:
      return &gliderfieldList[index];
    case OutLandingList:
      return &outLandingList[index];
    case RadioList:
      return &radioList[index];
    case AirspaceList:
      return airspaceList.at(index);
    case ObstacleList:
      return &obstacleList[index];
    case ReportList:
      return &reportList[index];
    case CityList:
      return &cityList[index];
    case VillageList:
      return &villageList[index];
    case LandmarkList:
      return &landmarkList[index];
    case MotorwayList:
      return &motorwayList[index];
    case RoadList:
      return &roadList[index];
    case RailList:
      return &railList[index];
    case HydroList:
      return &hydroList[index];
    case LakeList:
      return &lakeList[index];
    case TopoList:
      return &topoList[index];
    default:
      // Should never happen!
      qCritical("Cumulus: trying to access unknown map element list");
      return static_cast<BaseMapElement *> (0);
    }
}

SinglePoint* MapContents::getSinglePoint(int listIndex, unsigned int index)
{
  switch (listIndex)
    {
    case AirfieldList:
      return static_cast<SinglePoint *> (&airfieldList[index]);
    case GliderfieldList:
      return static_cast<SinglePoint *> (&gliderfieldList[index]);
    case OutLandingList:
      return static_cast<SinglePoint *> (&outLandingList[index]);
    case RadioList:
      return static_cast<SinglePoint *> (&radioList[index]);
    case ObstacleList:
      return &obstacleList[index];
    case ReportList:
      return &reportList[index];
    case VillageList:
      return &villageList[index];
    case LandmarkList:
      return &landmarkList[index];
    default:
      // Should never happen!
      qCritical("Cumulus: trying to access unknown map element list");
      return static_cast<SinglePoint *> (0);
    }
}


/** This slot is called to do a first load of all map data or to do a
 *  reload of certain map data after a position move or projection change. */
void MapContents::slotReloadMapData()
{
  // @AP: defined a static mutex variable, to prevent the recursive
  // calling of this method
  static bool mutex = false;

  if ( mutex )
    {
      // qDebug("MapContents::slotReloadMapData(): mutex is locked, returning");
      return; // return immediately, if reentry in method is not possible
    }

  mutex = true;

  // We must block all GPS signals during the reload time to avoid
  // system crash due to out dated data.
  GpsNmea::gps->enableReceiving( false );

  // clear the airspace path list in map too
  Map::getInstance()->clearAirspaceRegionList();

  qDeleteAll(airspaceList);
  airspaceList.clear();
  cityList.clear();
  hydroList.clear();
  lakeList.clear();
  landmarkList.clear();
  radioList.clear();
  obstacleList.clear();
  railList.clear();
  reportList.clear();
  motorwayList.clear();
  roadList.clear();
  topoList.clear();
  villageList.clear();

  welt2000Mutex.lock();

  airfieldList.clear();
  gliderfieldList.clear();
  outLandingList.clear();

  // free internal allocated memory in QList
  airfieldList    = QList<Airfield>();
  gliderfieldList = QList<Airfield>();
  outLandingList  = QList<Airfield>();

  welt2000Mutex.unlock();

  // all isolines are cleared
  groundMap.clear();
  terrainMap.clear();

  // tile maps are cleared
  tileSectionSet.clear();
  tilePartMap.clear();

  isFirst  = true;
  isReload = true;

  // Reload all data, that must be always done after a projection data change
  proofeSection();

  // Check for a selected waypoint, this one must be also new projected.
  extern Calculator  *calculator;
  extern MapContents *_globalMapContents;
  extern MapMatrix   *_globalMapMatrix;

  // Update the global selected waypoint
  Waypoint *wp = (Waypoint *) calculator->getselectedWp();

  if ( wp )
    {
      wp->projP = _globalMapMatrix->wgsToMap(wp->origP);
    }

  // Update the global waypoint list
  for ( int loop = 0; loop < wpList.count(); loop++ )
    {
      // recalculate projection data
      wpList[loop].projP = _globalMapMatrix->wgsToMap(wpList[loop].origP);
    }

  // Make a recalculation of the reachable sites. The projection of
  // all collected sites have been changed.
  calculator->newSites();

  // Check for a flight task, the waypoint list must be updated too
  FlightTask *task = _globalMapContents->getCurrentTask();

  if ( task != 0 )
    {
      task->updateProjection();
    }

  // that signal will update all list views of the main window
  emit mapDataReloaded();

  // enable gps data receiving
  GpsNmea::gps->ignoreConnectionLost();
  GpsNmea::gps->enableReceiving( true );

  mutex = false; // unlock mutex
}

#ifdef WELT2000_THREAD

/**
 * Starts a thread, which is loading the requested Welt2000 data.
 */
void MapContents::loadWelt2000DataViaThread()
{
  QMutexLocker locker( &welt2000Mutex );

  _globalMapView->slot_info( tr("loading Welt2000") );

  Welt2000Thread *w2000Thread = new Welt2000Thread( this );

  // Register a special data type for return results. That must be
  // done to transfer the results between different threads.
  qRegisterMetaType<AirfieldListPtr>("AirfieldListPtr");

  // Connect the receiver of the results. It is located in this
  // thread and not in the new opened thread.
  connect( w2000Thread,
           SIGNAL(loadedLists( bool,
                               QList<Airfield>*,
                               QList<Airfield>*,
                               QList<Airfield>*  )),
           this,
           SLOT(slotWelt2000LoadFinished( bool,
                                          QList<Airfield>*,
                                          QList<Airfield>*,
                                          QList<Airfield>* )) );
  w2000Thread->start();
}

/**
 * This slot is called by the Welt2000 load thread to signal, that the
 * requested airfield data have been loaded. The passed lists must be
 * deleted in this method.
 */
void MapContents::slotWelt2000LoadFinished( bool ok,
                                            QList<Airfield>* airfieldListIn,
                                            QList<Airfield>* gliderfieldListIn,
                                            QList<Airfield>* outlandingListIn )
{
  QMutexLocker locker( &welt2000Mutex );

  if( ok == false )
    {
      qDebug() << "slotWelt2000LoadFinished: Welt2000 loading failed!";

      _globalMapView->slot_info( tr("Welt2000 load failed") );

      delete airfieldListIn;
      delete gliderfieldListIn;
      delete outlandingListIn;
      return;
    }

  // Take over the new loaded lists. The passed lists must be deleted!
  airfieldList = *airfieldListIn;
  delete airfieldListIn;

  gliderfieldList = *gliderfieldListIn;
  delete gliderfieldListIn;

  outLandingList = *outlandingListIn;
  delete outlandingListIn;

  _globalMapView->slot_info( tr("Welt2000 loaded") );

  emit mapDataReloaded();
}

#endif

/**
 * Reload the Welt2000 data file. Can be called after a configuration change or
 * a file download.
 */
void MapContents::slotReloadWelt2000Data()
{
#ifndef WELT2000_THREAD

  // @AP: defined a static mutex variable, to prevent the recursive
  // calling of this method
  static bool mutex = false;

  if ( mutex )
    {
      return; // return immediately, if reentry in method is not possible
    }

  mutex = true;

  // We must block all GPS signals during the reload time to avoid
  // system crash due to outdated data.
  GpsNmea::gps->enableReceiving( false );

  airfieldList.clear();
  gliderfieldList.clear();
  outLandingList.clear();

  _globalMapView->slot_info( tr("loading Welt2000") );

  Welt2000 welt2000;
  welt2000.load( airfieldList, gliderfieldList, outLandingList );

  _globalMapView->slot_info( tr("Welt2000 loaded") );

  emit mapDataReloaded();

  // enable GPS data receiving
  GpsNmea::gps->ignoreConnectionLost();
  GpsNmea::gps->enableReceiving( true );

  mutex = false; // unlock mutex

#else

  // Reload Welt2000 data in an extra thread.
  loadWelt2000DataViaThread();

#endif
}

/**
 * Reloads the airspace data files. Can be called after a configuration change
 * or a download.
 */
void MapContents::slotReloadAirspaceData()
{
  // @AP: defined a static mutex variable, to prevent the recursive
  // calling of this method
  static bool mutex = false;

  if ( mutex )
    {
      return; // return immediately, if reentry in method is not possible
    }

  mutex = true;

  // We must block all GPS signals during the reload time to avoid
  // system crash due to outdated data.
  GpsNmea::gps->enableReceiving( false );

  // clear the airspace path list in map too
  Map::getInstance()->clearAirspaceRegionList();

  qDeleteAll(airspaceList);
  airspaceList.clear();

  // free all internal allocated memory in QList
  airspaceList = SortableAirspaceList();

  _globalMapView->slot_info( tr("loading Airspaces") );

  OpenAirParser oap;
  oap.load( airspaceList );

  // finally, sort the airspaces
  airspaceList.sort();

  _globalMapView->slot_info( tr("Airspaces loaded") );

  emit mapDataReloaded();

  // enable GPS data receiving
  GpsNmea::gps->ignoreConnectionLost();
  GpsNmea::gps->enableReceiving( true );

  mutex = false; // unlock mutex
}

/** Special method to add the drawn objects to the return list,
 * if the required option is set.
 */
void MapContents::drawList( QPainter* targetP,
                            unsigned int listID,
                            QList<Airfield*> &drawnAfList )
{
  //const char *list="";
  //uint len = 0;

  //QTime t;
  //t.start();

  // load all configuration items once
  const bool showAfLabels  = GeneralConfig::instance()->getMapShowAirfieldLabels();
  const bool showOlLabels  = GeneralConfig::instance()->getMapShowOutLandingLabels();

  switch (listID)
    {
    case AirfieldList:
      //list="AirfieldList";
      //len=airfieldList.count();
      showProgress2WaitScreen( tr("Drawing airports") );

      for (int i = 0; i < airfieldList.size(); i++)
        {
          if(  airfieldList[i].drawMapElement(targetP) && showAfLabels )
            {
              // required and draw object is appended to the list
              drawnAfList.append( &airfieldList[i] );
            }
        }

      break;

    case GliderfieldList:
      //list="GliderList";
      //len=gliderfieldList.count();
      showProgress2WaitScreen( tr("Drawing glider sites") );

      for (int i = 0; i < gliderfieldList.size(); i++)
        {
          if( gliderfieldList[i].drawMapElement(targetP) && showAfLabels )
            {
              // required and draw object is appended to the list
              drawnAfList.append( &gliderfieldList[i] );
            }
        }

      break;

    case OutLandingList:
      //list="outLandingList";
      //len=outLandingList.count();
      showProgress2WaitScreen( tr("Drawing outlanding sites") );

      for (int i = 0; i < outLandingList.size(); i++)
        {
          if( outLandingList[i].drawMapElement(targetP) && showOlLabels )
            {
              // required and draw object is appended to the list
              drawnAfList.append( &outLandingList[i] );
            }
        }

      break;

    default:
      qWarning("MapContents::drawList(): unknown listID %d", listID);
      return;
    }

  // qDebug( "List=%s, Length=%d, drawTime=%dms", list, len, t.elapsed() );
}

void MapContents::drawList( QPainter* targetP,
                            unsigned int listID,
                            QList<BaseMapElement *>& drawnElements )
{
  //const char *list="";
  //uint len = 0;

  //QTime t;
  //t.start();

  switch (listID)
    {
    case AirfieldList:
      //list="AirfieldList";
      //len=airfieldList.count();
      showProgress2WaitScreen( tr("Drawing airports") );

      for (int i = 0; i < airfieldList.size(); i++)
        {
          airfieldList[i].drawMapElement(targetP);
        }

      break;

    case GliderfieldList:
      //list="GliderList";
      //len=gliderfieldList.count();
      showProgress2WaitScreen( tr("Drawing glider sites") );

      for (int i = 0; i < gliderfieldList.size(); i++)
        {
          gliderfieldList[i].drawMapElement(targetP);
        }

      break;

    case OutLandingList:
      //list="outLandingList";
      //len=outLandingList.count();
      showProgress2WaitScreen( tr("Drawing outlanding sites") );

      for (int i = 0; i < outLandingList.size(); i++)
        {
          outLandingList[i].drawMapElement(targetP);
        }

      break;

    case RadioList:
      //list="RadioList";
      //len=radioList.count();
      showProgress2WaitScreen( tr("Drawing radio points") );
      for (int i = 0; i < radioList.size(); i++)
        radioList[i].drawMapElement(targetP);
      break;

    case AirspaceList:
      //list="AirspaceList";
      //len=airspaceList.count();
      showProgress2WaitScreen( tr("Drawing airspaces") );
      for (int i = 0; i < airspaceList.size(); i++)
        airspaceList.at(i)->drawMapElement(targetP);
      break;

    case ObstacleList:
      //list="ObstacleList";
      //len=obstacleList.count();
      showProgress2WaitScreen( tr("Drawing obstacles") );
      for (int i = 0; i < obstacleList.size(); i++)
        obstacleList[i].drawMapElement(targetP);
      break;

    case ReportList:
      //list="ReportList";
      //len=reportList.count();
      showProgress2WaitScreen( tr("Drawing reporting points") );
      for (int i = 0; i < reportList.size(); i++)
        reportList[i].drawMapElement(targetP);
      break;

    case CityList:
      //list="CityList";
      //len=cityList.count();
      showProgress2WaitScreen( tr("Drawing cities") );
      for (int i = 0; i < cityList.size(); i++)
        {
          if( cityList[i].drawMapElement(targetP) )
            {
              drawnElements.append( &cityList[i] );
            }
        }

      break;

    case VillageList:
      //list="VillageList";
      //len=villageList.count();
      showProgress2WaitScreen( tr("Drawing villages") );
      for (int i = 0; i < villageList.size(); i++)
        villageList[i].drawMapElement(targetP);
      break;

    case LandmarkList:
      //list="LandmarkList";
      //len=landmarkList.count();
      showProgress2WaitScreen( tr("Drawing landmarks") );
      for (int i = 0; i < landmarkList.size(); i++)
        landmarkList[i].drawMapElement(targetP);
      break;

    case MotorwayList:
      //list="MotorwayList";
      //len=MotorwayList.count();
      showProgress2WaitScreen( tr("Drawing motorways") );
      for (int i = 0; i < motorwayList.size(); i++)
        motorwayList[i].drawMapElement(targetP);
      break;

    case RoadList:
      //list="RoadList";
      //len=roadList.count();
      showProgress2WaitScreen( tr("Drawing roads") );
      for (int i = 0; i < roadList.size(); i++)
        roadList[i].drawMapElement(targetP);
      break;

    case RailList:
      //list="RailList";
      //len=railList.count();
      showProgress2WaitScreen( tr("Drawing railroads") );
      for (int i = 0; i < railList.size(); i++)
        railList[i].drawMapElement(targetP);
      break;

    case HydroList:
      //list="HydroList";
      //len=hydroList.count();
      showProgress2WaitScreen( tr("Drawing hydro") );
      for (int i = 0; i < hydroList.size(); i++)
        hydroList[i].drawMapElement(targetP);
      break;

    case LakeList:
      //list="LakeList";
      //len=lakeList.count();
      showProgress2WaitScreen( tr("Drawing lakes") );
      for (int i = 0; i < lakeList.size(); i++)
        lakeList[i].drawMapElement(targetP);
      break;

    case TopoList:
      //list="TopoList";
      //len=topoList.count();
      showProgress2WaitScreen( tr("Drawing topography") );
      for (int i = 0; i < topoList.size(); i++)
        topoList[i].drawMapElement(targetP);
      break;

    default:
      qWarning("MapContents::drawList(): unknown listID %d", listID);
      return;
    }

  // qDebug( "List=%s, Length=%d, drawTime=%dms", list, len, t.elapsed() );
}

/**
 * Draws the isoline areas and the related outer isoline border. Drawing
 * size depends on user configuration:
 *
 * a) Only ground can be drawn
 * b) Ground and terrain can be drawn
 * c) Isoline borders can be drawn depending on map scale. If map scale > 160
 *    drawing will be switched off automatically.
 **/
void MapContents::drawIsoList(QPainter* targetP)
{
  // qDebug("MapContents::drawIsoList():");

  QTime t;
  t.start();

  extern MapMatrix* _globalMapMatrix;
  _lastIsoEntry = 0;
  _isoLevelReset = true;
  pathIsoLines.clear();
  bool isolines = false;
  GeneralConfig *conf = GeneralConfig::instance();

  targetP->setPen(QPen(Qt::black, 1, Qt::NoPen));

  if( conf->getMapShowIsoLineBorders() )
    {
      int scale = (int) rint(_globalMapMatrix->getScale(MapMatrix::CurrentScale));

      if( scale < 160 )
        {
          // Draw outer isolines only at lower scales
          isolines = true;
        }
    }

  targetP->save();
  showProgress2WaitScreen( tr("Drawing surface contours") );

  int count = 1; // draw only the ground

  bool drawTerrain = conf->getMapLoadIsoLines();

  // shall we draw the terrain too?
  if( drawTerrain )
    {
      count++; // yes
    }

  int elevationIndexOffest = GeneralConfig::instance()->getElevationColorOffset();

  QMap< int, QList<Isohypse> >* isoMaps[2] = { &groundMap, &terrainMap };

  for( int i = 0; i < count; i++ )
    {
      // assign the map to be drawn to the iterator
      QMapIterator<int, QList<Isohypse> > it(*isoMaps[i]);

      while (it.hasNext())
        {
          // Iterate over all tiles in the isomap and fetch the isoline lists.
          // The isoline list contains all isolines of a tile in ascending order.
          it.next();

          // Check, if tile has a map overlapping otherwise we can ignore it
          // completely.
          QRect mapBorder = _globalMapMatrix->getViewBorder();

          if( getTileBox( it.key() ).intersects(mapBorder) == false )
            {
              // qDebug("Tile=%d do not intersect", it.key() );
              continue;
            }

          const QList<Isohypse> &isoList = it.value();

         for (int j = 0; j < isoList.size(); j++)
            {
              Isohypse isoLine = isoList.at(j);

              if( drawTerrain )
                {
                  // Choose contour color.
                  // The index of the isoList has a fixed relation to the isocolor list
                  // normally with an offset of one.
                  int colorIdx = isoLine.getElevationIndex();

                  // We can move the color index by a user configuration option
                  // to get a better color schema.
                  if( elevationIndexOffest != 0 && i == 1 )
                    {
                      int newIndex = colorIdx + elevationIndexOffest;

                      if( newIndex >= 0 && newIndex <= SIZEOF_TERRAIN_COLORS )
                        {
                          // Move color index to the new position
                          colorIdx = newIndex;
                        }
                      else if( newIndex < 0 )
                        {
                          colorIdx = 0;
                        }
                      else if( newIndex >=  SIZEOF_TERRAIN_COLORS )
                        {
                          colorIdx = SIZEOF_TERRAIN_COLORS - 1;
                        }
                    }

                  targetP->setBrush( QBrush(conf->getTerrainColor(colorIdx),
                                      Qt::SolidPattern));
                }
              else
                {
                  // Only ground level will be drawn. We take the ground color
                  // when isoline drawing is switched off by the user.
                  targetP->setBrush( QBrush(conf->getGroundColor(),
                                      Qt::SolidPattern));
                }

              // draw the single isoline
              QPainterPath* Path = isoLine.drawRegion( targetP, isolines);

              if (Path)
                {
                  // store drawn path in extra list for elevation finding
                  IsoListEntry entry(Path, isoLine.getElevation());
                  pathIsoLines.append(entry);
                  //qDebug("  added Iso: %04x, %d", (int)reg, iso2.getElevation() );
                }
            }
        }
    }

  targetP->restore();
  pathIsoLines.sort();
  _isoLevelReset = false;

  qDebug( "IsoList, drawTime=%dms", t.elapsed() );

#if 0
  QString isos;

  for ( int l = 0; l < pathIsoLines.count(); l++ )
    {
      isos += QString("%1, ").arg(pathIsoLines.at(l).height);
    }

  qDebug( isos.toLatin1().data() );
#endif
}

/**
 * shows a progress message at the wait screen, if it is visible
 */
void MapContents::showProgress2WaitScreen( QString message )
{
  if ( ws && ws->isVisible() )
    {
      ws->slot_SetText1( message );
      ws->slot_Progress(1);
    }
}

/** This function checks all possible map directories for the map
    file. If found, it returns true and returns the complete path in
    pathName. */
bool MapContents::locateFile(const QString& fileName, QString& pathName)
{
  QStringList mapDirs = GeneralConfig::instance()->getMapDirectories();

  for ( int i = 0; i < mapDirs.size(); ++i )
    {
      QFile test;

      test.setFileName( mapDirs.at(i) + "/" + fileName );

      if ( test.exists() )
        {
          pathName=test.fileName();
          return true;
        }
    }

  // lower case tests
  for ( int i = 0; i < mapDirs.size(); ++i )
    {
      QFile test;

      test.setFileName( mapDirs.at(i) + "/" + fileName.toLower() );

      if ( test.exists() )
        {
          pathName=test.fileName();
          return true;
        }
    }

  // so, let's try upper case
  for ( int i = 0; i < mapDirs.size(); ++i )
    {
      QFile test;

      test.setFileName( mapDirs.at(i) + "/" + fileName.toUpper() );

      if ( test.exists() )
        {
          pathName=test.fileName();
          return true;
        }
    }

  return false;
}


void MapContents::addDir (QStringList& list, const QString& _path, const QString& filter)
{
  //  qDebug ("addDir (%s, %s)", _path.toLatin1().data(), filter.toLatin1().data());
  QDir path (_path, filter);

  //JD was a bit annoyed by many notifications about nonexisting dirs
  if ( ! path.exists() )
    return;

  QStringList entries (path.entryList());

  for (QStringList::Iterator it = entries.begin(); it != entries.end(); ++it )
    {
      bool found = false;
      // look for other entries with same filename
      for (QStringList::Iterator it2 =  list.begin(); it2 != list.end(); ++it2)
        {
          QFileInfo path2 (*it2);
          if (path2.fileName() == *it)
            found = true;
        }
      if (!found)
        list += path.absoluteFilePath (*it);
    }
  //  qDebug ("entries: %s", list.join(";").toLatin1().data());
}


/** Read property of FlightTask * currentTask. */
FlightTask* MapContents::getCurrentTask()
{
  return currentTask;
}


/** Take over a new FlightTask as current task. Note, that passed
 *  task can be null, if it is reset. */
void MapContents::setCurrentTask( FlightTask* _newVal)
{
  // an old task instance must be deleted
  if ( currentTask != 0 )
    {
      delete currentTask;
    }

  currentTask = _newVal;

  // Set declaration date-time in flight task. Is used by the IGC logger in the
  // C record as declaration date-time.
  if( _newVal )
    {
      currentTask->setDeclarationDateTime();
    }
}


/** Returns true if the coordinates of the waypoint in the argument
 * matches one of the waypoints in the list. */
bool MapContents::isInWaypointList(const QPoint& wgsCoord)
{
  for (int i=0; i < wpList.count(); i++)
    {
      const Waypoint& wpItem = wpList.at(i);

      if ( wgsCoord == wpItem.origP )
        {
          return true;
        }
    }

  return false;
}

/**
 * @Returns true if the name of the waypoint in the argument
 * matches one of the waypoints in the list.
 */
bool MapContents::isInWaypointList(const QString& name )
{
  for (int i=0; i < wpList.count(); i++)
    {
      const Waypoint& wpItem = wpList.at(i);

      if ( name == wpItem.name )
        {
          return true;
        }
    }

  return false;
}

/**
 * @Returns how often the name of the waypoint in the argument
 * matches one of the waypoints in the list.
 */
unsigned short MapContents::countNameInWaypointList( const QString& name )
{
  ushort number = 0;

  for (int i=0; i < wpList.count(); i++)
    {
      const Waypoint& wpItem = wpList.at(i);

      if ( name == wpItem.name )
        {
          number++;;
        }
    }

  return number;

}


QDateTime MapContents::getDateFromMapFile( const QString& path )
{
  QDateTime createDateTime;
  QFile mapFile( path );
  if (!mapFile.open(QIODevice::ReadOnly))
    {
      qWarning("Cumulus: can't open map file %s for reading date", path.toLatin1().data() );
      createDateTime.setDate( QDate(1900,1,1) );
      return createDateTime;
    }

  QDataStream in(&mapFile);
  in.setVersion(QDataStream::Qt_2_0);

  mapFile.seek( 9 );
  in >> createDateTime;
  mapFile.close();
  //qDebug("Map file %s created %s", path.toLatin1().data(), createDateTime.toString().toLatin1().data() );
  return createDateTime;
}


/** Add a point to a rectangle, so the rectangle will be the bounding box
 * of all points added to it. If the point already lies within the borders
 * of the QRect, the QRect is unchanged. If the point is outside the
 * defined QRect, the QRox will be modified so the point lies inside the
 * new QRect. If the QRect is empty, the QRect will be set to a rectangle of
 * size (1,1) at the location of the point. */
void MapContents::AddPointToRect(QRect& rect, const QPoint& point)
{
  if (rect.isValid())
    {
      rect.setCoords(
        qMin(rect.left(), point.x()),
        qMin(rect.top(), point.y()),
        qMax(rect.right(), point.x()),
        qMax(rect.bottom(), point.y()));
    }
  else
    {
      rect.setCoords(point.x(),point.y(),point.x(),point.y());
    }
}


/**
 * Compares two projection objects for equality.
 * @Returns true if equal; otherwise false
 */
bool MapContents::compareProjections(ProjectionBase* p1, ProjectionBase* p2)
{
  if ( p1->projectionType() != p2->projectionType() )
    {
      return false;
    }

  if ( p1->projectionType() == ProjectionBase::Lambert )
    {
      ProjectionLambert* l1 = (ProjectionLambert *) p1;
      ProjectionLambert* l2 = (ProjectionLambert *) p2;

      if ( l1->getStandardParallel1() != l2->getStandardParallel1() ||
           l1->getStandardParallel2() != l2->getStandardParallel2() ||
           l1->getOrigin() != l2->getOrigin() )
        {
          return false;
        }

      return true;
    }

  if ( p1->projectionType() == ProjectionBase::Cylindric )
    {
      ProjectionCylindric* c1 = (ProjectionCylindric*) p1;
      ProjectionCylindric* c2 = (ProjectionCylindric*) p2;

      if ( c1->getStandardParallel() != c2->getStandardParallel() )
        {
          return false;
        }

      return true;
    }

  // What's that? Det kennen wir noch nicht :( Rejection!

  return false;
}

int MapContents::findElevation(const QPoint& coordP, Distance* errorDist)
{
  extern MapMatrix* _globalMapMatrix;

  const IsoListEntry* entry = 0;
  int height = 0;
  double error = 0.0;

  QPoint coordP1 = _globalMapMatrix->wgsToMap(coordP.x(), coordP.y());
  QPoint coord = _globalMapMatrix->map(coordP1);

  IsoList* list = getIsohypseRegions();

  // qDebug("list->count() %d", list->count() );
  //qDebug("==searching for elevation==");
  //qDebug("_lastIsoLevel %d, _nextIsoLevel %d",_lastIsoLevel, _nextIsoLevel);
  if (_isoLevelReset)
    {
      qDebug("findElevation: Busy rebuilding the isomap. Returning last known result...");
      return _lastIsoLevel;
    }

  int cnt = list->count();

  for ( int i=0; i<cnt; i++ )
    {
      entry = &(list->at(i));
      // qDebug("i: %d entry->height %d contains %d",i,entry->height, entry->path->contains(coord) );
      // qDebug("Point x:%d y:%d", coord.x(), coord.y() );
      // qDebug("boundingRect l:%d r:%d t:%d b:%d", entry->path->boundingRect().left(),
      //                                 entry->path->boundingRect().right(),
      //                                 entry->path->boundingRect().top(),
      //                                 entry->path->boundingRect().bottom() );

      if (entry->height > height && /*there is no reason to search a lower level if we already have a hit on a higher one*/
          entry->height <= _nextIsoLevel) /* since the odds of skipping a level between two fixes are not too high, we can ignore higher levels, making searching more efficient.*/
        {
          if (entry->height == _lastIsoLevel && _lastIsoEntry)
            {
              //qDebug("Trying previous entry...");
              if (_lastIsoEntry->path->contains(coord))
                {
                  height = qMax(height, entry->height);
                  //qDebug("Found on height %d",entry->height);
                  break;
                }
            }

          if (entry == _lastIsoEntry)
            {
              continue; //we already tried this one, and it wasn't it.
            }

          //qDebug("Probing on height %d...", entry->height);

          if (entry->path->contains(coord))
            {
              height = qMax(height,entry->height);
              //qDebug("Found on height %d",entry->height);
              _lastIsoEntry = entry;
              break;
            }
        }
    }

  _lastIsoLevel = height;

  // The real altitude is between the current and the next
  // isolevel, therefore reduce error by taking the middle
  if ( height <100 )
    {
      _nextIsoLevel = height+25;
      height += 12;
      error=12.5;
    }
  else if ( (height >=100) && (height < 500) )
    {
      _nextIsoLevel = height+50;
      height += 25;
      error=25.0;
    }
  else if ( (height >=500) && (height < 1000) )
    {
      _nextIsoLevel = height+100;
      height += 50;
      error=50.0;
    }
  else
    {
      _nextIsoLevel = height+250;
      height += 125;
      error = 125.0;
    }

  // if errorDist is set, set the correct error margin
  if (errorDist)
    {
      errorDist->setMeters(error);
    }

  return height;
}
