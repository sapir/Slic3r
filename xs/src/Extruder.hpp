#ifndef slic3r_Extruder_hpp_
#define slic3r_Extruder_hpp_

#include <myinit.h>
#include "Point.hpp"
#include "PrintConfig.hpp"

namespace Slic3r {

class Extruder
{
    public:
    Extruder(int id, PrintConfig *config);
    virtual ~Extruder() {}
    void reset();
    double extrude(double dE);
    
    
    Pointf extruder_offset() const;
    double nozzle_diameter() const;
    double filament_diameter() const;
    double extrusion_multiplier() const;
    int temperature() const;
    int first_layer_temperature() const;
    double retract_length() const;
    double retract_lift() const;
    int retract_speed() const;
    double retract_restart_extra() const;
    double retract_before_travel() const;
    bool retract_layer_change() const;
    double retract_length_toolchange() const;
    double retract_restart_extra_toolchange() const;
    bool wipe() const;


    int retract_speed_mm_min() const;

    // how far do we move in XY at travel_speed for the time needed to consume
    // retract_length at retract_speed?
    double scaled_wipe_distance(double travel_speed) const;


    int id;
    double E;
    double absolute_E;
    double retracted;
    double restart_extra;
    
    PrintConfig *config;
};

}

#endif
