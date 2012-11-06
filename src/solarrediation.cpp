#include "solarrediation.h"
#include <math.h>
#include <iostream>
double SolarRediation::BeamRadiation(unsigned int dayOftheYear, double elevation, double altitude, double alpha)
{
    double T_LK = 1; //Linke aotmosheric tubidity factor
    const double pi =  3.14159265;
    const double I_0  = 1367; // Solar constant W/m2

    double j_day  = 2 * pi * dayOftheYear/365.25; //Day angle
    double epsilon = 1 + 0.03344 * cos (j_day - 0.048869); // correction factor

    double G_0 = I_0 * epsilon;



    double ptop0 = exp(-elevation / 8434.5);
    double delta_h_0Ref = (0.1594 + altitude * (1.123 + 0.065656 * altitude)) /
            (1. + altitude * (28.9344 + 277.3971 * altitude));
    double h0ref = altitude +  0.061359 * delta_h_0Ref; //rad


    double m = ptop0 / (sin(h0ref) + 0.50572 * pow(h0ref *180 / pi + 6.07995, -1.6364));

    double delta_R = 0;
     if (m > 20)
         delta_R = 1/(10.4 + 0.718 * m);
     else
         delta_R = 1. / (6.6296 + m * (1.7513 + m * (-0.1202 + m * (0.0065 - m * 0.00013))));


    double B_0c = G_0  * exp (-0.8662*T_LK * m * delta_R); // irradiance normal to the solar beam

    double radiation =  B_0c * sin(alpha); //W/m2

    return radiation;

}
