#include "simple_playlist.hpp"


namespace ion
{


metadata_optional_t simple_playlist::get_metadata_for(uri const &uri_) const
{
	return metadata_t(Json::objectValue);
}


uri_optional_t simple_playlist::get_succeeding_uri(uri const &uri_) const
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::const_iterator uri_tag_iter = entries_by_uri.find(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return boost::none;

	typedef entries_t::index < sequence_tag > ::type entry_sequence_t;
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = entries.project < sequence_tag > (uri_tag_iter);
	++seq_iter;

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
		return seq_iter->uri_;
}


void simple_playlist::mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
{
}


void simple_playlist::add_entry(entry const &entry_)
{
	entries.push_back(entry_);
	resource_added_signal(entry_.uri_);
}


void simple_playlist::remove_entry(entry const &entry_)
{
	ion::uri uri_ = entry_.uri_;

	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = entries_by_uri.find(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	// NOTE: erase must happen before the signal is emitted, because otherwise a transition event may be handled incorrectly (it may still see the entry)
	// TODO: to solve this, use a mutex
	entries_by_uri.erase(uri_tag_iter);
	resource_removed_signal(uri_);
}


uint64_t simple_playlist::get_num_entries() const
{
	return entries.size();
}


simple_playlist::entry const * simple_playlist::get_entry(uint64_t const nr) const
{
	if (nr >= get_num_entries())
		return 0;

	typedef entries_t::index < sequence_tag > ::type entries_sequence_t;
	entries_sequence_t const &entries_sequence = entries.get < sequence_tag > ();
	entries_sequence_t::const_iterator iter = entries_sequence.begin();
	iter = iter + nr;
	return &(*iter);
}


simple_playlist::entry const * simple_playlist::get_entry(uri const &uri_) const
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::const_iterator iter = entries_by_uri.find(uri_);

	if (iter != entries_by_uri.end())
		return &(*iter);
	else
		return 0;
}


boost::optional < uint64_t > simple_playlist::get_entry_index(uri const &uri_) const
{
	typedef entries_t::index < sequence_tag > ::type entries_sequence_t;
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;

	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::const_iterator uri_iter = entries_by_uri.find(uri_);
	if (uri_iter == entries_by_uri.end())
		return boost::none;

	entries_sequence_t const &entries_sequence = entries.get < sequence_tag > ();
	entries_sequence_t::const_iterator sequence_iter = entries.project < sequence_tag > (uri_iter);
	return uint64_t(sequence_iter - entries_sequence.begin());
}




simple_playlist::resource_event_signal_t & get_resource_added_signal(simple_playlist &playlist)
{
	return playlist.get_resource_added_signal();
}


simple_playlist::resource_event_signal_t & get_resource_removed_signal(simple_playlist &playlist)
{
	return playlist.get_resource_removed_signal();
}


metadata_optional_t get_metadata_for(simple_playlist const &playlist, uri const &uri_)
{
	return playlist.get_metadata_for(uri_);
}


uri_optional_t get_succeeding_uri(simple_playlist const &playlist, uri const &uri_)
{
	return playlist.get_succeeding_uri(uri_);
}


void mark_backend_resource_incompatibility(simple_playlist &playlist, uri const &uri_, std::string const &backend_type)
{
	playlist.mark_backend_resource_incompatibility(uri_, backend_type);
}


}

