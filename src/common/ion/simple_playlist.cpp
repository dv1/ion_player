#include <iostream>
#include <json/value.h>
#include "simple_playlist.hpp"


namespace ion
{


simple_playlist::~simple_playlist()
{
	// TODO: if an URI of this playlist is currently playing, send the stop command
	std::cerr << "~simple_playlist" << std::endl;
}


metadata_optional_t simple_playlist::get_metadata_for(uri const &uri_) const
{
	return metadata_t(Json::objectValue);
}


simple_playlist::entries_t::index < simple_playlist::sequence_tag > ::type::const_iterator simple_playlist::get_seq_iterator_for(uri const &uri_) const
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;

	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();

	entries_by_uri_t::const_iterator uri_tag_iter = entries_by_uri.find(uri_);
	if (uri_tag_iter == entries_by_uri.end())
		return entry_sequence.end();

	return entries.project < sequence_tag > (uri_tag_iter);
}


simple_playlist::entries_t::index < simple_playlist::sequence_tag > ::type::iterator simple_playlist::get_seq_iterator_for(uri const &uri_)
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;

	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entry_sequence_t &entry_sequence = entries.get < sequence_tag > ();

	entries_by_uri_t::iterator uri_tag_iter = entries_by_uri.find(uri_);
	if (uri_tag_iter == entries_by_uri.end())
		return entry_sequence.end();

	return entries.project < sequence_tag > (uri_tag_iter);
}


uri_optional_t simple_playlist::get_succeeding_uri(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = get_seq_iterator_for(uri_);

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		++seq_iter;
		if (seq_iter == entry_sequence.end())
			return boost::none;
		else
			return seq_iter->uri_;
	}
}


uri_optional_t simple_playlist::get_preceding_uri(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = get_seq_iterator_for(uri_);

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		if (seq_iter == entry_sequence.begin())
			return boost::none;
		else
		{
			--seq_iter;
			return seq_iter->uri_;
		}
	}
}


void simple_playlist::mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
{
}


simple_playlist::unique_id_optional_t simple_playlist::get_uri_id(uri const &uri_)
{
	uri::options_t::const_iterator iter = uri_.get_options().find("id");
	if (iter == uri_.get_options().end())
		return boost::none;
	else
	{
		try
		{
			return boost::lexical_cast < unique_ids_t::id_t > (iter->second);
		}
		catch (boost::bad_lexical_cast const &)
		{
			return boost::none;
		}
	}
}


void simple_playlist::set_uri_id(uri &uri_, unique_ids_t::id_t const &new_id, bool const check_for_old_id)
{
	if (check_for_old_id)
	{
		unique_id_optional_t old_id = get_uri_id(uri_);
		if (old_id)
			unique_ids_.erase(*old_id);
	}

	uri_.get_options()["id"] = boost::lexical_cast < std::string > (new_id);
}


void simple_playlist::add_entry(entry entry_, bool const emit_signal) // NOT using entry const &entry_ to allow uri modifications
{
	unique_id_optional_t old_id = get_uri_id(entry_.uri_);
	if (old_id)
		unique_ids_.insert(*old_id);
	else
		set_uri_id(entry_.uri_, unique_ids_.create_new(), false);

	entries.push_back(entry_);
	if (emit_signal)
		resource_added_signal(entry_.uri_);
}


void simple_playlist::insert_entry_before(entry entry_, entry_sequence_t::iterator insert_before_iter, bool const emit_signal)
{
	unique_id_optional_t old_id = get_uri_id(entry_.uri_);
	if (old_id)
		unique_ids_.insert(*old_id);
	else
		set_uri_id(entry_.uri_, unique_ids_.create_new(), false);

	entry_sequence_t &entry_sequence = entries.get < sequence_tag > ();

	std::pair < entry_sequence_t::iterator, bool > insert_result = entry_sequence.insert(insert_before_iter, entry_);
	if (!insert_result.second)
		std::cerr << "simple_playlist:insert_entry_before(): insertion was prevented" << std::endl;

	if (emit_signal)
		resource_added_signal(entry_.uri_);
}


void simple_playlist::insert_entry_before(entry const &entry_, uri const &insert_before_uri, bool const emit_signal)
{
	entry_sequence_t::iterator insert_before_iter = get_seq_iterator_for(insert_before_uri);
	insert_entry_before(entry_, insert_before_iter, emit_signal);
}


void simple_playlist::insert_entry_after(entry const &entry_, uri const &insert_after_uri, bool const emit_signal)
{
	entry_sequence_t::iterator insert_before_iter = get_seq_iterator_for(insert_after_uri);
	++insert_before_iter;
	insert_entry_before(entry_, insert_before_iter, emit_signal);
}


void simple_playlist::remove_entry(entry const &entry_, bool const emit_signal)
{
	unique_id_optional_t id = get_uri_id(entry_.uri_);
	if (id)
		unique_ids_.erase(*id);

	remove_entry(entry_.uri_, emit_signal);
}


void simple_playlist::remove_entry(uri const &uri_, bool const emit_signal)
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = entries_by_uri.find(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	// NOTE: erase must happen before the signal is emitted, because otherwise a transition event may be handled incorrectly (it may still see the entry)
	// TODO: to solve this, use a mutex
	entries_by_uri.erase(uri_tag_iter);

	if (emit_signal)
		resource_removed_signal(uri_);
}


void simple_playlist::set_resource_metadata(uri const &uri_, metadata_t const & new_metadata)
{
	typedef entries_t::index < uri_tag > ::type entries_by_uri_t;
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = entries_by_uri.find(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	simple_playlist::entry entry_ = *uri_tag_iter;

	entries_by_uri.erase(uri_tag_iter);

	entry_.metadata = new_metadata;
	entries.push_back(entry_);

	resource_metadata_changed_signal(uri_);
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


simple_playlist::entry_range_t simple_playlist::get_entry_range() const
{
	typedef entries_t::index < sequence_tag > ::type entries_sequence_t;
	entries_sequence_t const &entries_sequence = entries.get < sequence_tag > ();

	return entry_range_t(entries_sequence.begin(), entries_sequence.end());
}




void load_from(simple_playlist &playlist_, Json::Value const &in_value)
{
	// TODO: clear existing contents from the playlist

	for (unsigned int index = 0; index < in_value.size(); ++index)
	{
		Json::Value json_entry = in_value[index];

		try
		{
			simple_playlist::entry entry_(ion::uri(json_entry["uri"].asString()), json_entry["metadata"]);
			playlist_.add_entry(entry_, true);
		}
		catch (ion::uri::invalid_uri const &invalid_uri_)
		{
			std::cerr << "Detected invalid uri \"" << invalid_uri_.what() << '"' << std::endl;
		}
	}

	// TODO: send a signal that the playlist's contents have changed entirely
}


void save_to(simple_playlist const &playlist_, Json::Value &out_value)
{
	out_value = Json::Value(Json::arrayValue);

	simple_playlist::entry_range_t entry_range = playlist_.get_entry_range();
	for (simple_playlist::entry_range_t::iterator iter = entry_range.begin(); iter != entry_range.end(); ++iter)
	{
		simple_playlist::entry const &entry_ = *iter;
		Json::Value json_entry(Json::objectValue);
		json_entry["uri"] = entry_.uri_.get_full();
		json_entry["metadata"] = entry_.metadata;

		out_value.append(json_entry);
	}
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


uri_optional_t get_preceding_uri(simple_playlist const &playlist, uri const &uri_)
{
	return playlist.get_preceding_uri(uri_);
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

