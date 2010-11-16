/****************************************************************************

Copyright (c) 2010 Carlos Rafael Giani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

****************************************************************************/


#include <boost/assign/list_of.hpp>
#include "flat_playlist.hpp"


namespace ion
{


flat_playlist::flat_playlist(unique_ids_t &unique_ids_):
	unique_ids_(unique_ids_)
{
}


metadata_optional_t flat_playlist::get_metadata_for(uri const &uri_) const
{
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return empty_metadata();
	else
		return boost::fusion::at_c < 1 > (*uri_tag_iter);
}


flat_playlist::entries_by_uri_t::const_iterator flat_playlist::get_uri_iterator_for(uri const &uri_) const
{
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::const_iterator uri_tag_iter = entries_by_uri.find(uri_);

	return uri_tag_iter;
}


flat_playlist::entries_by_uri_t::iterator flat_playlist::get_uri_iterator_for(uri const &uri_)
{
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = entries_by_uri.find(uri_);

	return uri_tag_iter;
}


uri_optional_t flat_playlist::get_preceding_uri(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = entries.project < sequence_tag > (get_uri_iterator_for(uri_));

	if (entry_sequence.empty())
		return boost::none;

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		if (seq_iter == entry_sequence.begin())
		{
			if (repeating)
				seq_iter = entry_sequence.end();
			else
				return boost::none;
		}

		--seq_iter;
		return boost::fusion::at_c < 0 > (*seq_iter);
	}
}


uri_optional_t flat_playlist::get_succeeding_uri(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = entries.project < sequence_tag > (get_uri_iterator_for(uri_));

	if (entry_sequence.empty())
		return boost::none;

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		++seq_iter;
		if (seq_iter == entry_sequence.end())
		{
			if (repeating)
				seq_iter = entry_sequence.begin();
			else
				return boost::none;
		}

		return boost::fusion::at_c < 0 > (*seq_iter);
	}
}


void flat_playlist::mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
{
	entry_sequence_t &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::iterator seq_iter = entries.project < sequence_tag > (get_uri_iterator_for(uri_));

	if (seq_iter != entry_sequence.end())
	{
		entry_t entry = *seq_iter;
		boost::fusion::at_c < 2 > (entry) = true;
		entry_sequence.replace(seq_iter, entry);
		resource_incompatible_signal(uri_);
	}
}


flat_playlist::num_entries_t flat_playlist::get_num_entries() const
{
	return entries.size();
}


flat_playlist::entry_t const * flat_playlist::get_entry(index_t const index) const
{
	if (index >= get_num_entries())
		return 0;

	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator iter = entry_sequence.begin();
	iter = iter + index;
	return &(*iter);
}


flat_playlist::entry_t const * flat_playlist::get_entry(uri const &uri_) const
{
	entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();

	if (uri_tag_iter != entries_by_uri.end())
		return &(*uri_tag_iter);
	else
		return 0;
}


flat_playlist::index_optional_t flat_playlist::get_entry_index(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = entries.project < sequence_tag > (get_uri_iterator_for(uri_));

	if (seq_iter == entry_sequence.end())
		return boost::none;

	return index_t(seq_iter - entry_sequence.begin());
}


flat_playlist::entry_range_t flat_playlist::get_entry_range() const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	return entry_range_t(entry_sequence.begin(), entry_sequence.end());
}


void flat_playlist::add_entry(entry_t const & entry_, bool const emit_signal)
{
	entry_t local_entry_copy = entry_;

	ion::uri &uri_ = boost::fusion::at_c < 0 > (local_entry_copy);

	unique_id_optional_t old_id = get_uri_id(uri_);
	if (old_id)
		unique_ids_.insert(*old_id);
	else
		set_uri_id(uri_, unique_ids_.create_new(), false);

	if (emit_signal)
		resource_added_signal(boost::assign::list_of(uri_), true);

	entries.push_back(local_entry_copy);

	if (emit_signal)
		resource_added_signal(boost::assign::list_of(uri_), false);
}


void flat_playlist::remove_entry(entry_t const &entry_, bool const emit_signal)
{
	ion::uri const &uri_ = boost::fusion::at_c < 0 > (entry_);
	remove_entry(uri_, emit_signal);
}


void flat_playlist::remove_entry(uri const &uri_, bool const emit_signal)
{
	remove_entries(boost::assign::list_of(uri_), emit_signal);
}


void flat_playlist::remove_entries(uri_set_t const &uris, bool const emit_signal)
{
	if (emit_signal)
		resource_removed_signal(uris, true);

	BOOST_FOREACH(ion::uri const &uri_, uris)
	{
		entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
		entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);

		if (uri_tag_iter != entries_by_uri.end())
		{
			unique_id_optional_t id = get_uri_id(uri_);
			if (id)
				unique_ids_.erase(*id);

			std::cout << uri_.get_full() << std::endl;
			entries_by_uri.erase(uri_tag_iter);
		}
	}

	if (emit_signal)
		resource_removed_signal(uris, false);
}


void flat_playlist::set_resource_metadata(uri const &uri_, metadata_t const & new_metadata)
{
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	resource_metadata_changed_signal(boost::assign::list_of(uri_), true);

	entry_t entry_ = *uri_tag_iter;
	boost::fusion::at_c < 1 > (entry_) = new_metadata;
	entries_by_uri.replace(uri_tag_iter, entry_);

	resource_metadata_changed_signal(boost::assign::list_of(uri_), false);
}


void flat_playlist::clear_entries(bool const emit_signal)
{
	if (emit_signal)
		all_resources_changed_signal(true);

	entries.clear();

	if (emit_signal)
		all_resources_changed_signal(false);
}


flat_playlist::unique_id_optional_t flat_playlist::get_uri_id(uri const &uri_)
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


void flat_playlist::set_uri_id(uri &uri_, unique_ids_t::id_t const &new_id, bool const check_for_old_id)
{
	if (check_for_old_id)
	{
		unique_id_optional_t old_id = get_uri_id(uri_);
		if (old_id)
			unique_ids_.erase(*old_id);
	}

	uri_.get_options()["id"] = boost::lexical_cast < std::string > (new_id);
}




void flat_playlist::load_from(Json::Value const &in_value)
{
	set_name(in_value["name"].asString());
	set_repeating(in_value.get("repeating", false).asBool());

	get_all_resources_changed_signal()(true);
	clear_entries(false);

	Json::Value entries_value = in_value.get("entries", Json::Value(Json::objectValue));
	for (unsigned int index = 0; index < entries_value.size(); ++index)
	{
		Json::Value json_entry = entries_value[index];

		try
		{
			flat_playlist::entry_t entry_(ion::uri(json_entry["uri"].asString()), json_entry["metadata"], json_entry.get("incompatible", false).asBool());
			add_entry(entry_, true);
		}
		catch (ion::uri::invalid_uri const &invalid_uri_)
		{
			std::cerr << "Detected invalid uri \"" << invalid_uri_.what() << '"' << std::endl;
		}
	}

	get_all_resources_changed_signal()(false);
}


void flat_playlist::save_to(Json::Value &out_value) const
{
	out_value["name"] = get_name();
	out_value["type"] = "flat";
	out_value["repeating"] = is_repeating();

	Json::Value entries_value(Json::arrayValue);
	BOOST_FOREACH(flat_playlist::entry_t const &entry_, get_entry_range())
	{
		Json::Value entry_value(Json::objectValue);
		entry_value["uri"] = boost::fusion::at_c < 0 > (entry_).get_full();
		entry_value["metadata"] = boost::fusion::at_c < 1 > (entry_);
		entry_value["incompatible"] = boost::fusion::at_c < 2 > (entry_);
		entries_value.append(entry_value);
	}
	out_value["entries"] = entries_value;
}


}

