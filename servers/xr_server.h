/*************************************************************************/
/*  xr_server.h                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef XR_SERVER_H
#define XR_SERVER_H

#include "core/object/ref_counted.h"
#include "core/os/os.h"
#include "core/os/thread_safe.h"
#include "core/templates/rid.h"
#include "core/variant/variant.h"

class XRInterface;
class XRPositionalTracker;

/**
	The XR server is a singleton object that gives access to the various
	objects and SDKs that are available on the system.
	Because there can be multiple SDKs active this is exposed as an array
	and our XR server object acts as a pass through
	Also each positioning tracker is accessible from here.

	I've added some additional info into this header file that should move
	into the documentation, I will do so when we're close to accepting this PR
	or as a separate PR once this has been merged into the master branch.
**/

class XRServer : public Object {
	GDCLASS(XRServer, Object);
	_THREAD_SAFE_CLASS_

public:
	enum TrackerType {
		TRACKER_HEAD = 0x01, /* tracks the position of the players head (or in case of handheld AR, location of the phone) */
		TRACKER_CONTROLLER = 0x02, /* tracks a controller */
		TRACKER_BASESTATION = 0x04, /* tracks location of a base station */
		TRACKER_ANCHOR = 0x08, /* tracks an anchor point, used in AR to track a real live location */
		TRACKER_UNKNOWN = 0x80, /* unknown tracker */

		TRACKER_ANY_KNOWN = 0x7f, /* all except unknown */
		TRACKER_ANY = 0xff /* used by get_connected_trackers to return all types */
	};

	enum RotationMode {
		RESET_FULL_ROTATION = 0, /* we reset the full rotation, regardless of how the HMD is oriented, we're looking dead ahead */
		RESET_BUT_KEEP_TILT = 1, /* reset rotation but keep tilt. */
		DONT_RESET_ROTATION = 2, /* don't reset the rotation, we will only center on position */
	};

private:
	Vector<Ref<XRInterface>> interfaces;
	Dictionary trackers;

	Ref<XRInterface> primary_interface; /* we'll identify one interface as primary, this will be used by our viewports */

	double world_scale; /* scale by which we multiply our tracker positions */
	Transform3D world_origin; /* our world origin point, maps a location in our virtual world to the origin point in our real world tracking volume */
	Transform3D reference_frame; /* our reference frame */

	uint64_t last_process_usec; /* for frame timing, usec when we did our processing */
	uint64_t last_commit_usec; /* for frame timing, usec when we finished committing both eyes */
	uint64_t last_frame_usec; /* time it took between process and committing, we should probably average this over the last x frames */

protected:
	static XRServer *singleton;

	static void _bind_methods();

public:
	static XRServer *get_singleton();

	/*
		World scale allows you to specify a scale factor that is applied to all positioning vectors in our VR world in essence scaling up, or scaling down the world.
		For stereoscopic rendering specifically this is very important to give an accurate sense of scale.
		Add controllers into the mix and an accurate mapping of real world movement to perceived virtual movement becomes very important.

		Most VR platforms, and our assumption, is that 1 unit in our virtual world equates to 1 meter in the real mode.
		This scale basically effects the unit size relationship to real world size.

		I may remove access to this property in GDScript in favour of exposing it on the XROrigin3D node
	*/
	double get_world_scale() const;
	void set_world_scale(double p_world_scale);

	/*
		The world maps the 0,0,0 coordinate of our real world coordinate system for our tracking volume to a location in our
		virtual world. It is this origin point that should be moved when the player is moved through the world by controller
		actions be it straffing, teleporting, etc. Movement of the player by moving through the physical space is always tracked
		in relation to this point.

		Note that the XROrigin3D spatial node in your scene automatically updates this property and it should be used instead of
		direct access to this property and it therefore is not available in GDScript

		Note: this should not be used in AR and should be ignored by an AR based interface as it would throw what you're looking at in the real world
		and in the virtual world out of sync
	*/
	Transform3D get_world_origin() const;
	void set_world_origin(const Transform3D &p_world_origin);

	/*
		center_on_hmd calculates a new reference frame. This ensures the HMD is positioned to 0,0,0 facing 0,0,-1 (need to verify this direction)
		in the virtual world.

		You can ignore the tilt of the device ensuring you're looking straight forward even if the player is looking down or sideways.
		You can chose to keep the height the tracking provides which is important for room scale capable tracking.

		Note: this should not be used in AR and should be ignored by an AR based interface as it would throw what you're looking at in the real world
		and in the virtual world out of sync
	*/
	Transform3D get_reference_frame() const;
	void center_on_hmd(RotationMode p_rotation_mode, bool p_keep_height);

	/*
		get_hmd_transform gets our hmd transform (centered between eyes) with most up to date tracking, relative to the origin
	*/
	Transform3D get_hmd_transform();

	/*
		Interfaces are objects that 'glue' Godot to an AR or VR SDK such as the Oculus SDK, OpenVR, OpenHMD, etc.
	*/
	void add_interface(const Ref<XRInterface> &p_interface);
	void remove_interface(const Ref<XRInterface> &p_interface);
	int get_interface_count() const;
	Ref<XRInterface> get_interface(int p_index) const;
	Ref<XRInterface> find_interface(const String &p_name) const;
	Array get_interfaces() const;

	/*
		note, more then one interface can technically be active, especially on mobile, but only one interface is used for
		rendering. This interface identifies itself by calling set_primary_interface when it is initialized
	*/
	Ref<XRInterface> get_primary_interface() const;
	void set_primary_interface(const Ref<XRInterface> &p_primary_interface);

	/*
		Our trackers are objects that expose the orientation and position of physical devices such as controller, anchor points, etc.
		They are created and managed by our active AR/VR interfaces.
	*/
	void add_tracker(Ref<XRPositionalTracker> p_tracker);
	void remove_tracker(Ref<XRPositionalTracker> p_tracker);
	Dictionary get_trackers(int p_tracker_types);
	Ref<XRPositionalTracker> get_tracker(const StringName &p_name) const;

	/*
		We don't know which trackers and actions will existing during runtime but we can request suggested names from our interfaces to help our IDE UI.
	*/
	PackedStringArray get_suggested_tracker_names() const;
	PackedStringArray get_suggested_pose_names(const StringName &p_tracker_name) const;
	// Q: Should we add get_suggested_input_names and get_suggested_haptic_names even though we don't use them for the IDE?

	uint64_t get_last_process_usec();
	uint64_t get_last_commit_usec();
	uint64_t get_last_frame_usec();

	void _process();
	void _mark_commit();

	XRServer();
	~XRServer();
};

#define XR XRServer

VARIANT_ENUM_CAST(XRServer::TrackerType);
VARIANT_ENUM_CAST(XRServer::RotationMode);

#endif // XR_SERVER_H
