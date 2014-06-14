#include "GCode.hpp"


namespace Slic3r {


GCode::GCode(PlaceholderParser *placeholder_parser, int layer_count)
:   config(),
    placeholder_parser(placeholder_parser),
    standby_points(),   // empty vector
    enable_loop_clipping(true),
    enable_wipe(false),
    layer_count(layer_count),
    _layer_index(-1),
    layer(NULL),
    _layer_islands(NULL),
    _upper_layer_islands(NULL),
    _seam_position(),   // empty map
    shift_x(0),
    shift_y(0),
    z(0),
    extruders(),        // empty map
    multiple_extruders(false),
    extruder(0),
    external_mp(NULL),
    layer_mp(NULL),
    new_object(false),
    straight_once(true),
    elapsed_time(0),
    lifted(0),
    last_pos(0, 0),
    last_fan_speed(0),
    wipe_path()         // empty polyline
{
}

GCode::~GCode()
{
}


#ifdef SLIC3RXS
REGISTER_CLASS(GCode, "GCode");
#endif

}
