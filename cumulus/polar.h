/***************************************************************************
                          polar.h  -  description
                             -------------------
    begin                : Fre Okt 18 2002
    copyright            : (C) 2002 by Eggert Ehmke, 2008 Axel Pauli
    email                : eggert.ehmke@berlin.de, axel@kflog.org

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef POLAR_H
#define POLAR_H

#include <QWidget>
#include <QString>
#include "speed.h"

/**
 * @author Eggert Ehmke
 */
class Polar: public QObject
{
    Q_OBJECT
public:
    Polar(QObject*,
          const QString& name,const Speed& v1, const Speed& w1,
          const Speed& v2, const Speed& w2,
          const Speed& v3, const Speed& w3,
          double wingload, double wingarea,
          double emptyWeight, double grossWeight);
    
    Polar (const Polar&);

    virtual ~Polar();

    /**
     * set bug factor as percentage
     */
    void setWater (int water, int bugs);

    Speed getSink (const Speed& speed) const;

    /**
     * calculate best airspeed for given wind, lift and McCready value;
     */
    Speed bestSpeed (const Speed& wind, const Speed& lift, const Speed& mc) const;

    /**
     * calculate best glide ratio
     */
    double bestLD (const Speed& speed, const Speed& wind, const Speed& lift) const;

    /** draw a graphical polar on the given widget;
     * draw glide path according to lift, wind and McCready value
     */
    void drawPolar (QWidget* view, const Speed& wind,
                    const Speed& lift, const Speed& mc) const;

    inline QString name()const
    {
        return _name;
    };

    inline Speed v1()const
    {
        return _v1;
    };

    inline Speed w1()const
    {
        return _w1;
    };

    inline Speed v2()const
    {
        return _v2;
    };

    inline Speed w2()const
    {
        return _w2;
    };

    inline Speed v3()const
    {
        return _v3;
    };

    inline Speed w3()const
    {
        return _w3;
    };

    inline double emptyWeight()const
    {
        return _emptyWeight;
    };

    inline double grossWeight()const
    {
        return _grossWeight;
    };

    inline void setGrossWeight(double newValue)
    {
        _grossWeight = newValue;
    };

    inline int water()const
    {
        return _water;
    };

    inline int bugs()const
    {
        return _bugs;
    };

    inline int seats()const
    {
        return _seats;
    };

    inline void setSeats(int seats)
    {
        _seats=qMax(1, qMin(2, seats));
    };

    inline int maxWater()const
    {
        return _maxWater;
    };

    inline void setMaxWater(int liters)
    {
        _maxWater=qMax(0, liters);
    };


private:
    QString _name;
    Speed _v1;
    Speed _w1;
    Speed _v2;
    Speed _w2;
    Speed _v3;
    Speed _w3;
    // these are the parabel parameters used for approximation
    double _a, _aa, _b, _bb, _c, _cc;
    int _water;
    int _bugs;
    double _emptyWeight;
    double _grossWeight;
    int _seats;
    int _maxWater;
};

#endif
