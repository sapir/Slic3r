#ifndef slic3r_GCode_hpp_
#define slic3r_GCode_hpp_

#include <myinit.h>
#include <string>
#include "PrintConfig.hpp"
#include "Point.hpp"
#include "Polyline.hpp"


namespace Slic3r {


class Layer;
class PlaceholderParser;
class PrintObject;
class SurfaceCollection;


// draft for a binary representation of a G-code line

enum GCodeCmdType {
    gcctSyncMotion,
    gcctExtrude,
    gcctResetE,
    gcctSetTemp,
    gcctSetTempWait,
    gcctToolchange,
    gcctCustom
};

class GCodeCmd {
    public:
    GCodeCmdType type;
    float X, Y, Z, E, F;
    unsigned short T, S;
    std::string custom, comment;
    float xy_dist; // cache
    
    GCodeCmd(GCodeCmdType type)
        : type(type), X(0), Y(0), Z(0), E(0), F(0), T(-1), S(0), xy_dist(-1) {};
};


class GCodeMotionPlanner {
    public:
    GCodeMotionPlanner();
    ~GCodeMotionPlanner();
};


// GCode generator
class GCode {
    public:
    GCode(PlaceholderParser *placeholder_parser, int layer_count);
    ~GCode();

    FullPrintConfig config;
    PlaceholderParser *placeholder_parser;
    Points standby_points; // TODO: f? 3?
    bool enable_loop_clipping;
    // at least one extruder has wipe enabled
    bool enable_wipe;
    int layer_count;
    // just a counter
    int _layer_index;
    Layer *layer;
    SurfaceCollection *_layer_islands;
    SurfaceCollection *_upper_layer_islands;
    std::map<PrintObject*, Point> _seam_position; // TODO: object? point?
    coordf_t shift_x;
    coordf_t shift_y;
    coordf_t z;
    std::vector<int> extruders;
    bool multiple_extruders;
    int extruder;
    GCodeMotionPlanner *external_mp;
    GCodeMotionPlanner *layer_mp;
    bool new_object;
    bool straight_once;
    // in seconds
    double elapsed_time;
    double lifted;
    Point last_pos;
    double last_fan_speed;
    Polyline wipe_path;
};


}

#endif
