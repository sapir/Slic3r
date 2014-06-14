#ifndef slic3r_GCode_hpp_
#define slic3r_GCode_hpp_

#include <myinit.h>
#include <string>
#include "PrintConfig.hpp"
#include "Point.hpp"
#include "Polyline.hpp"
#include "ExPolygonCollection.hpp"


namespace Slic3r {


class Extruder;
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

    // TODO: these methods should probably be private/protected, and should
        // accept a stringstream reference so all of the gcode can be created
        // on a single stringstream
    std::string change_layer(Layer *layer);
    // this method accepts Z in unscaled coordinates
    std::string move_z(coordf_t to_z, std::string comment="");
    std::string set_acceleration(int acceleration);
    std::string retract(coordf_t move_z=0, bool toolchange=false);

    // TODO: these should be private, but Perl code still needs them
    std::string _G0_G1(bool is_G0,
        coordf_t e, coordf_t F, const std::string &comment);
    std::string _G0_G1(bool is_G0, Pointf point,
        coordf_t e, coordf_t F, const std::string &comment);
    std::string _G0_G1(bool is_G0, coordf_t z,
        coordf_t e, coordf_t F, const std::string &comment);
    std::string _G0_G1(bool is_G0, Pointf point, coordf_t z,
        coordf_t e, coordf_t F, const std::string &comment);


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
    ExPolygonCollection _layer_islands;
    ExPolygonCollection _upper_layer_islands;
    std::map<PrintObject*, Point> _seam_position; // TODO: object? point?
    coordf_t shift_x;
    coordf_t shift_y;
    coordf_t z;
    bool z_defined;
    std::map<int, Extruder*> extruders;
    bool multiple_extruders;
    Extruder *extruder;
    GCodeMotionPlanner *external_mp;
    GCodeMotionPlanner *layer_mp;
    bool new_object;
    bool straight_once;
    // in seconds
    double elapsed_time;
    double lifted;
    Pointf last_pos;
    double last_fan_speed;
    Polyline wipe_path;


    private:

    // doesn't output GCode
    void set_z(double value);

    // methods that accept a stringstream parameter assume that the
        // stream's precision is set to 3 and that the ios::fixed flag is set

    void out_pointf(std::stringstream &gcode_stm, const Pointf &point);
    // only outputs comment if enabled in configuration
    void out_comment(std::stringstream &gcode_stm, const std::string &comment);
    // may or may not actually use G0
    void do_Gx_gcode(std::stringstream &gcode_stm, bool is_G0);
    void do_Gx_point(std::stringstream &gcode_stm, const Pointf &point);
    void do_Gx_z(std::stringstream &gcode_stm, coordf_t z);
    void do_Gx_ending(std::stringstream &gcode_stm, coordf_t e, coordf_t F,
        const std::string &comment);
};


}

#endif
