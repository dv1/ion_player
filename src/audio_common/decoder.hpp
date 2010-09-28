#ifndef ION_AUDIO_COMMON_BACKEND_DECODER_HPP
#define ION_AUDIO_COMMON_BACKEND_DECODER_HPP

#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <ion/metadata.hpp>
#include <ion/resource_exceptions.hpp>
#include <ion/send_command_callback.hpp>
#include <ion/uri.hpp>

#include "types.hpp"


namespace ion
{
namespace audio_common
{


class decoder:
	private boost::noncopyable
{
public:
	typedef boost::optional < std::string > metadata_str_optional_t;


	virtual ~decoder()
	{
	}


	inline static long min_volume() { return 0; }
	inline static long max_volume() { return 16777215; }


	/**
	* Reports whether or not the decoder is in an initialized state, meaning that it's properly initialized and operational.
	* is_initialized() always returns the same; its return value solely depends on the outcome of the decoder initialization. In other words,
	* it never changes in time; operations on the decoder do not affect it.
	* It also does -not- depend on whether or not set_playback_properties() was ever called.
	* (can_playback() is essentially is_initialized() plus this extra check.)
	*
	* @return true if the decoder is initialized and operational, false otherwise
	*/
	virtual bool is_initialized() const = 0;


	/**
	* Reports whether or not the decoder can be used for playback, that is, update() would succeed.
	* This function is essentially is_initialized() plus an extra check for playback.
	* NOTE: when writing a custom sink, one must not rely on this function to determine whether or not a sink should stop playing. update()'s return value is the only relevant factor there.
	*
	* @return true if the decoder is initialized, operational and can be used for playback, false otherwise
	*/
	virtual bool can_playback() const = 0;


	/**
	* Pauses the decoder. Repeated calls are ignored.
	*
	* @pre The decoder must have been initialized properly, must be operating correctly, and not be in a paused state (if it is, the call will be ignored)
	* @post The decoder will be in a paused state
	*/
	virtual void pause() = 0;

	/**
	* Resumes the decoder. Repeated calls are ignored.
	*
	* @pre The decoder must have been initialized properly, must be operating correctly, and be in a paused state (if it is not, the call will be ignored)
	* @post The decoder will no longer be in a paused state
	*/
	virtual void resume() = 0;

	/**
	* Moves the current resource position to the one specified. The position is given in ticks. The decoder does _not_ have to guarantee that the current position
	* will exactly match the given one after this call is done (this may not be possible, depending on resource and decoder, sometimes one can only seek with coarse granularity).
	* The return value will be the new current position, which as described may differ from new_position.
	*
	* @param new_position The new position, in ticks
	* @return The new position, of -1 if setting the new position failed or is not possible (example: internet radio streams)
	* @pre new_position must be valid (>=0 and < get_num_ticks()); the decoder must be correctly initialized
	* @post The position will have changed if the precondition was met and the call was successful (= -1 was not returned); if it failed, nothing will have been changed
	*/
	virtual long set_current_position(long const new_position) = 0;

	/**
	* Returns the current position, in ticks.
	* @return The current position, or -1 if getting the position fails or is not possible (example: internet radio streams)
	*/
	virtual long get_current_position() const = 0;

	/**
	* Sets the current volume to the one specified. Valid range is 0 (silence) to 16777215 (full volume). The decoder does _not_ have to guarantee that the current volume
	* will exactly match the given one after this call is done (this may not be possible, depending on resource and decoder, sometimes one can only set the volume with coarse granularity).
	* The return value will be the new current volume, which as described may differ from new_volume.
	*
	* @param new_volume The new volume; valid range = 0 (silence) to 16777215 (full volume)
	* @return The new current volume
	* @pre new_volume must be within the valid range; the decoder must be correctly initialized
	* @post The current volume will have been changed if the precondition was met; if not, this call will be ignored
	*/
	virtual long set_current_volume(long const new_volume) = 0;

	/**
	* Returns the current volume. The valid range goes from 0 (silence) to 16777215 (full volume).
	* @return The current volume
	*/
	virtual long get_current_volume() const = 0;

	/**
	* Gets the metadata for the resource that is decoded.
	* @return Metadata structure
	*/
	virtual metadata_t get_metadata() const = 0;

	/**
	* Gets the type of this decoder. This is used for an RTTI like functionality.
	* @return The type of this decoder, as a string (this type does not have to equal the C++ type name, an unambigous type identification is enough)
	*/
	virtual std::string get_type() const = 0;

	/**
	* Get the uri of the resource this decoder is decoding/will decode.
	* @pre the decoder must be correctly initialized
	* @return The resource uri, or uri() if the decoder is not initialized properly
	*/
	virtual uri get_uri() const = 0;

	/**
	* Gets the amount of ticks available through this decoder. The amount of ticks normally equals the resource length. In certain cases, such an amount of ticks is not available
	* (example: live audio broadcasts); this function will return 0 then.
	*
	* @pre the decoder must be correctly initialized
	* @return Amount of ticks the resource has, or 0 if this information is unavailable
	*/
	virtual long get_num_ticks() const = 0;

	/**
	* Gets the amount of ticks that make up one second. This information is necessary to convert the return value of get_num_ticks() to seconds: the result of get_num_ticks()/get_num_ticks_per_second()
	* would be the resource length in seconds. In cases where the amount of ticks is unavailable (as described for get_num_ticks()), this function returns -1.
	*
	* @pre the decoder must be correctly initialized
	* @return Amount of ticks that make up one second, or -1 if this information is unavailable
	*/
	virtual long get_num_ticks_per_second() const = 0;

	/**
	* Sets the loop mode. See the set_loop_mode command for details.
	* @pre: nothing.
	* @post: the loop mode will be set accordingly.
	*/
	virtual void set_loop_mode(int const new_loop_mode) = 0;

	/**
	* Sets playback properties, telling the decoder what the output should be like.
	* Note that there is no room for tolerance here; the decoder MUST ensure the output matches these properties.
	* If necessary, convert in the decoder. The one exception is the samplerate, which may be ignored. In this case, get_decoder_samplerate() must return a meaningful
	* value. The sink will then resample.
	*
	* @param new_playback_properties The new playback properties to use
	* @pre new_playback_properties must be valid; if not, this call will not do anything
	* @post The decoder will emit data as described by the properties
	* @inv Volume, and pause state must not be affected by this. Song position does not have to be affected (it can be, but only if it is unavoidable;
	*      in some decoders, a reset may be necessary, with a subsequent seek to the previous position not being possible)
	*/
	virtual void set_playback_properties(playback_properties const &new_playback_properties) = 0;

	/**
	* Returns the decoder's samplerate. If the decoder performs internal resampling, then this function must return the resampled data's frequency, NOT the original one.
	* If said resampling converts the data to match the sample rate specified in set_playback_properties(), then this must return either 0 or said sample rate.
	* In other words, the sink checks if the return value of this function is 0 or the sink's playback sample rate. In this case, the sink chooses not to resample.
	* Otherwise, it does.
	*
	* @pre: nothing.
	* @return The decoder's samplerate.
	*/
	virtual unsigned int get_decoder_samplerate() const = 0;

	/**
	* Updates the dest buffer with new samples. num_samples_to_write specifies how many samples to write.
	* Note that the amound of samples does not depend on the amount of channels. So, don't calculate num_samples * num_channels.
	* The return value equals the amount of samples that were actually computed. Once zero is returned, the decoder will not return any more data.
	* Subsequent update() calls will then always return zero (but still be valid calls). Consider zero as sort of an end-of-file indicator.
	*
	* IMPORTANT: in implementations, this function MUST be thread safe, since this function will almost always be used in a different thread by the sink. Thread safety between update() and
	* the othe functions in this decoder class must be guaranteed, that is, for example, it should be safe to call set_current_position() while the sink is calling update() in another thread.
	*
	* @param dest Pointer to start of the destination sample buffer, where the decoder shall write the samples to; this buffer must be at least num_samples_to_write * num_channels * sample_size_in_bytes big.
	* @param num_samples_to_write Amount of samples to write to the sample buffer. Note that in ion, the unit "sample" ignores the amount of channels. Thus, if you say "256 samples" in a mono setup (1 channel),
	* it will still be 256 samples in a stereo setup (2 channels). For buffer size calculations, you have to take this into account, as described for dest.
	* @return Amount of samples that have actually been written.
	* @pre dest and num_samples_to_write must be valid; the decoder must be correctly initialized
	* @post up to num_samples_to_write samples will have been written in dest; the actual amount equals the return value.
	* @inv This function does not have to modify internal states, but is free to do so.
	*/
	virtual unsigned int update(void *dest, unsigned int const num_samples_to_write) = 0;


	/**
	* Returns the send command callback set for this decoder.
	* @return The send command callback this decoder is using
	*/
	send_command_callback_t get_send_command_callback() const { return send_command_callback; }


	/**
	* Gets the song info as a string. The song info itself is already in JSON form; this is a convenience function that serializes the JSON value to string.
	* @return String representation of the JSON value containing the song info, or boost::none if an error occurred
	*/
	metadata_str_optional_t get_metadata_str(bool const fill_autogenerated_data = true) const
	{
		metadata_t metadata_ = get_metadata();

		if (fill_autogenerated_data)
		{
			unsigned int num_ticks = get_num_ticks();
			unsigned int num_ticks_per_second = get_num_ticks_per_second();

			if (num_ticks_per_second == 0)
				throw resource_corrupted(get_uri().get_full());

			set_metadata_value(metadata_, "decoder_type",         get_type());
			set_metadata_value(metadata_, "num_ticks",            int(num_ticks));
			set_metadata_value(metadata_, "num_ticks_per_second", int(num_ticks_per_second));

			{
				std::string title = get_metadata_value(metadata_, "title", std::string(""));
				if (title.empty())
					set_metadata_value(metadata_, "title", get_uri().get_basename());
			}
		}

		return get_metadata_string(metadata_);
	}


protected:
	explicit decoder(send_command_callback_t const &send_command_callback):
		send_command_callback(send_command_callback)
	{
	}


	send_command_callback_t send_command_callback;
};


typedef boost::shared_ptr < decoder > decoder_ptr_t;


}
}


#endif

