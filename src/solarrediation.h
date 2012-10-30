#ifndef SOLARREDIATION_H
#define SOLARREDIATION_H

class SolarRediation
{
public:
    static double BeamRadiation(unsigned int dayOftheYear, double elevation, double altitude, double alpha);

};

#endif // SOLARREDIATION_H
