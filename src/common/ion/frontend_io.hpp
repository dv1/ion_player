#ifndef ION_FRONTEND_IO_HPP
#define ION_FRONTEND_IO_HPP

#include <boost/noncopyable.hpp>


namespace ion
{


/*

This class is used for communicating with a backend. More specifically, it sends commands, receives events, and reacts to the latter.
It does -not- directly start a backend; instead, it abstracts away I/O with the backend by using a function and a callback.
- The parse_incoming_line() function is to be called whenever the backend process sent a line
- The send_line_to_backend callback will be used whenever this class wants to send the backend a command

frontend_io is used as a part of a system. Said system performs the actual backend startup, and controls its I/O streams (typically stdin
and stdout).


The frontend io's logic can be grouped in three categories:

- actions: tellng the backend to play, stop, ..
- backend event handlers: when the backend sends an event, these react accordingly; example: transition event handler
- playlist event handlers: these are called by the playlist, to inform the frontend io about playlist changes that are relevant to the current playback
  for example, when the currently playing resource gets deleted from the playlist, the backend should immediately transition to the next resource



=== actions:
These are simple. They just send a command to the backend, nothing more.

* play: gets an uri as parameter. It then asks the playlist for a uri that succeeds the given one, provided one exists.
  The playlist is also asked to provide metadata for the uri(s).
  Then the play command is sent to the backend. If a succeeding uri was retrieved, it is put in the play command's parameter.
  As for the metadata: for each uri the playlist didn't have metadata for, a get_metadata command is sent as well.

* stop: just sends the stop command.



=== backend events:

* metadata: information about a resource's metadata. The handler tells the playlist about said metadata.

* transition: has uris for the previous and the next resource. The next resource is about to start playing, the previous one
  is being phased out. This handler looks up a succeeding uri for the the next resource. If one is found, it asks the playlist
  for metadata for it, and sends a set_next_resource command, with the found uri and its metadata as parameter. frontend_io's
  current_uri and next_uri values are changed as well; current_uri is set to next_uri's value, and next_uri is set to the succeeding
  uri. If no succeeding uri was found, then next_uri is nulled. In any way, listeners are notified about the change in current_uri's
  value.

* started: informs about playback start. This is not sent by the backend when a transition happens. It has one or two parameters,
  the currently playing resource's uri, and the uri of the next resource. The latter is not present if no next resource was
  specified to the backend.
  Upon receiving this event, the handler sets frontend_io's internal current_uri and next_uri values, and notifies listeners
  that frontend_io's current uri just got changed.

* stopped: sent when playback was stopped by the user. NOT sent when a transition happens (see the transition event) or the resource
  finished with no next resource set (see the resource_finished event). The current_uri and next_uri values get nulled, and listeners
  are notified about current_uri's value change.

* resource_finished: very similar to stopped, except that this is sent by the backend when the resource finished playing with no next
  resource specified. It is NOT set when the user stopped playback (see the stopped event) or when a transition happens (see the
  transition event).



=== playlist event handlers

* resource added: a resource just got added to the playlist. The handler checks if the new resource has been inserted -before- the
  one that was set as the next resource. If this is the case, then the next resource is set to this new one.

* resource removed: a resource just got removed from the playlist. Two cases need to be handled:
  - the removed resource might be the one that was previously set as the next resource. In this case, the next resource needs to be
    determined again by asking the playlist for the uri that succeeds the currently playing resource's. set_next_resource is then
    sent to the backend, either with the uri the playlist returned, or with an empty string as parameter if no such uri was found.
  - The removed resource might be the currently playling resource's. In this case, tell the backend to immediately initiate a
    transition. The rest is just regular transition logic.




Additional actions and event handlers are front/backend specific. Examples are pause, resume, set/get volume, set/get position,
set/get current subsong, get settings ui, ....

*/


class frontend_io:
	private boost::noncopyable
{
public:
protected:
};


}


#endif

