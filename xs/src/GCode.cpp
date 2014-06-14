#include "GCode.hpp"
#include "Extruder.hpp"
#include "Layer.hpp"
#include <sstream>


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
    _layer_islands(),       // empty collection
    _upper_layer_islands(), // empty collection
    _seam_position(),       // empty map
    shift_x(0),
    shift_y(0),
    z(0),
    z_defined(false),
    extruders(),        // empty map
    multiple_extruders(false),
    extruder(NULL),
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

std::string
GCode::change_layer(Layer *layer)
{
    this->layer = layer;
    ++this->_layer_index;

    // avoid computing islands and overhangs if they're not needed
    this->_layer_islands = layer->islands();
    this->_upper_layer_islands =
        (NULL == layer->upper_layer)
        ? ExPolygonCollection()
        : layer->upper_layer->islands();

    if (this->config.avoid_crossing_perimeters) {
        this->layer_mp = new GCodeMotionPlanner();
            // TODO:islands => union_ex([ map @$_, @{$layer->slices} ], 1),
    }

    std::stringstream gcode_stm;
    if (this->config.gcode_flavor == gcfMakerWare
        || this->config.gcode_flavor == gcfSailfish)
    {
        // TODO: cap this to 99% and add an explicit M73 P100 in the end G-code
        gcode_stm << "M73 P"
            << int(99 * (this->_layer_index / (float)(this->layer_count - 1)));
        out_comment(gcode_stm, "update progress");
        gcode_stm << "\n";
    }

    if (this->config.first_layer_acceleration) {
        if (layer->id() == 0) {
            gcode_stm << this->set_acceleration(this->config.first_layer_acceleration);
        } else if (layer->id() == 1) {
            gcode_stm << this->set_acceleration(this->config.default_acceleration);
        }
    }

    gcode_stm << this->move_z(layer->print_z);

    return gcode_stm.str();
}

void
GCode::set_z(double value)
{
    this->z = value;
    this->z_defined = true;
}

void
GCode::out_pointf(std::stringstream &gcode_stm, const Pointf &point)
{
    gcode_stm
        << " X" << (point.x * SCALING_FACTOR) + this->shift_x - this->extruder->extruder_offset().x
        << " Y" << (point.y * SCALING_FACTOR) + this->shift_y - this->extruder->extruder_offset().y;
}

void
GCode::out_comment(std::stringstream &gcode_stm, const std::string &comment)
{
    if (!comment.empty() && this->config.gcode_comments) {
        gcode_stm << " ; " << comment;
    }
}

void
GCode::do_Gx_gcode(std::stringstream &gcode_stm, bool is_G0)
{
    gcode_stm <<
        (is_G0 && (this->config.g0 || this->config.gcode_flavor == gcfMach3))
        ? "G0"
        : "G1";
}

void
GCode::do_Gx_point(std::stringstream &gcode_stm, const Pointf &point)
{
    out_pointf(gcode_stm, point);
    this->last_pos = point;
}

void
GCode::do_Gx_z(std::stringstream &gcode_stm, coordf_t z)
{
    if (!this->z_defined || z != this->z) {
        this->set_z(z);
        gcode_stm << " Z" << this->z;
    }
}

void
GCode::do_Gx_ending(std::stringstream &gcode_stm, coordf_t e, coordf_t F,
    const std::string &comment)
{
    gcode_stm << "F" << F;

    // output extrusion distance
    if (0 != e) {
        std::string eaxis = this->config.get_extrusion_axis();
        if (!eaxis.empty()) {
            gcode_stm << " " << eaxis << this->extruder->extrude(e);
        }
    }

    out_comment(gcode_stm, comment);

    gcode_stm << "\n";
}

std::string
GCode::_G0_G1(bool use_G0,
    coordf_t e, coordf_t F, const std::string &comment)
{
    std::stringstream gcode_stm;
    gcode_stm.setf(std::ios::fixed);
    gcode_stm.precision(3);
    this->do_Gx_gcode(gcode_stm, use_G0);
    this->do_Gx_ending(gcode_stm, e, F, comment);
    return gcode_stm.str();
}

std::string
GCode::_G0_G1(bool use_G0, Pointf point,
    coordf_t e, coordf_t F, const std::string &comment)
{
    std::stringstream gcode_stm;
    gcode_stm.setf(std::ios::fixed);
    gcode_stm.precision(3);
    this->do_Gx_gcode(gcode_stm, use_G0);
    this->do_Gx_point(gcode_stm, point);
    this->do_Gx_ending(gcode_stm, e, F, comment);
    return gcode_stm.str();
}

std::string
GCode::_G0_G1(bool use_G0, coordf_t z,
    coordf_t e, coordf_t F, const std::string &comment)
{
    std::stringstream gcode_stm;
    gcode_stm.setf(std::ios::fixed);
    gcode_stm.precision(3);
    this->do_Gx_gcode(gcode_stm, use_G0);
    this->do_Gx_z(gcode_stm, z);
    this->do_Gx_ending(gcode_stm, e, F, comment);
    return gcode_stm.str();
}

std::string
GCode::_G0_G1(bool use_G0, Pointf point, coordf_t z,
    coordf_t e, coordf_t F, const std::string &comment)
{
    std::stringstream gcode_stm;
    gcode_stm.setf(std::ios::fixed);
    gcode_stm.precision(3);
    this->do_Gx_gcode(gcode_stm, use_G0);
    this->do_Gx_point(gcode_stm, point);
    this->do_Gx_z(gcode_stm, z);
    this->do_Gx_ending(gcode_stm, e, F, comment);
    return gcode_stm.str();
}

std::string
GCode::move_z(coordf_t to_z, std::string comment)
{
    std::stringstream gcode_stm;

    to_z += this->config.z_offset;

    // nominal_z is relevant only if z_defined is true
    coordf_t nominal_z = this->z - this->lifted;
    if (!this->z_defined || to_z > this->z || to_z < nominal_z) {
        // we're moving above the current actual Z (so above the lift height of the current
        // layer if any) or below the current nominal layer

        // in both cases, we're going to the nominal Z of the next layer
        this->lifted = 0;

        if (this->extruder->retract_layer_change()) {
            // this retraction may alter this->z
            gcode_stm << this->retract(to_z, false);

            // update in case retract() changed this->z
            nominal_z = this->z - this->lifted;
        }

        if (!this->z_defined ||  std::abs(to_z - nominal_z) > EPSILON) {
            if (comment.empty()) {
                std::stringstream comment_stm;
                comment_stm << "move to next layer (" << this->layer->id() << ")";
                comment = comment_stm.str();
            }

            gcode_stm << this->_G0_G1(true, to_z,
                0,      // E
                this->config.travel_speed*60, // F
                comment);
        }

    // this->z_defined is actually implied by previous if stmt
    } else if (this->z_defined && to_z < this->z) {
        // we're moving above the current nominal layer height and below the current actual one.
        // we're basically advancing to next layer, whose nominal Z is still lower than the previous
        // layer Z with lift.
        this->lifted = this->z - to_z;
    }

    return gcode_stm.str();
}

std::string
GCode::set_acceleration(int acceleration)
{
    if (0 == acceleration) {
        return "";
    }

    std::stringstream gcode_stm;
    gcode_stm << "M204 S" << acceleration;
    out_comment(gcode_stm, "adjust acceleration");
    return gcode_stm.str();
}

std::string
GCode::retract(coordf_t move_z, bool toolchange)
{
    // get the retraction length and abort if none
    double length;
    double restart_extra;
    std::string comment;
    if (toolchange) {
        length = this->extruder->retract_length_toolchange();
        restart_extra = this->extruder->retract_restart_extra_toolchange();
        comment = "retract for tool change";
    } else {
        length = this->extruder->retract_length();
        restart_extra = this->extruder->retract_restart_extra();
        comment = "retract";
    }

    // if we already retracted, reduce the required amount of retraction
    length -= this->extruder->retracted;
    if (length <= 0) {
        return "";
    }

    std::stringstream gcode_stm;
    // TODO
#if 0
    // wipe
    my $wipe_path;
    if ($self->extruder->wipe && $self->wipe_path) {
        my @points = @{$self->wipe_path};
        $wipe_path = Slic3r::Polyline->new($self->last_pos, @{$self->wipe_path}[1..$#{$self->wipe_path}]);
        $wipe_path->clip_end($wipe_path->length - $self->extruder->scaled_wipe_distance($self->config->travel_speed));
    }

    // prepare moves
    my $retract = [undef, undef, -$length, $self->extruder->retract_speed_mm_min, $comment];
    my $lift    = ($self->config->retract_lift->[0] == 0 || defined $params{move_z}) && !$self->lifted
        ? undef
        : [undef, $self->z + $self->config->retract_lift->[0], 0, $self->config->travel_speed*60, 'lift plate during travel'];

    // check that we have a positive wipe length
    if ($wipe_path) {
        // subdivide the retraction
        my $retracted = 0;
        foreach my $line (@{$wipe_path->lines}) {
            my $segment_length = $line->length;
            // reduce retraction length a bit to avoid effective retraction speed to be greater than the configured one
            // due to rounding
            my $e = $retract->[2] * ($segment_length / $self->extruder->scaled_wipe_distance($self->config->travel_speed)) * 0.95;
            $retracted += $e;
            $gcode .= $self->G1($line->b, undef, $e, $self->config->travel_speed*60*0.8, $retract->[3] . ";_WIPE");
        }
        if ($retracted > $retract->[2]) {
            // if we retracted less than we had to, retract the remainder
            // TODO: add regression test
            $gcode .= $self->G1(undef, undef, $retract->[2] - $retracted, $self->extruder->retract_speed_mm_min, $comment);
        }
    } elsif ($self->config->use_firmware_retraction) {
        $gcode .= "G10 ; retract\n";
    } else {
        $gcode .= $self->G1(@$retract);
    }
    if (!$self->lifted) {
        if (defined $params{move_z} && $self->config->retract_lift->[0] > 0) {
            my $travel = [undef, $params{move_z} + $self->config->retract_lift->[0], 0, $self->config->travel_speed*60, 'move to next layer (' . $self->layer->id() . ') and lift'];
            $gcode .= $self->G0(@$travel);
            $self->lifted($self->config->retract_lift->[0]);
        } elsif ($lift) {
            $gcode .= $self->G1(@$lift);
        }
    }
    $self->extruder->set_retracted($self->extruder->retracted + $length);
    $self->extruder->set_restart_extra($restart_extra);
    $self->lifted($self->config->retract_lift->[0]) if $lift;

    // reset extrusion distance during retracts
    // this makes sure we leave sufficient precision in the firmware
    $gcode .= $self->reset_e;

    $gcode .= "M103 ; extruder off\n" if $self->config->gcode_flavor eq 'makerware';
#endif

    return gcode_stm.str();
}


#ifdef SLIC3RXS
REGISTER_CLASS(GCode, "GCode");
#endif

}
