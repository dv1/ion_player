#include <boost/assign/list_of.hpp>
#include "flat_playlist.hpp"


namespace ion
{


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

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		if (seq_iter == entry_sequence.begin())
			return boost::none;
		else
		{
			--seq_iter;
			return boost::fusion::at_c < 0 > (*seq_iter);
		}
	}
}


uri_optional_t flat_playlist::get_succeeding_uri(uri const &uri_) const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator seq_iter = entries.project < sequence_tag > (get_uri_iterator_for(uri_));

	if (seq_iter == entry_sequence.end())
		return boost::none;
	else
	{
		++seq_iter;
		if (seq_iter == entry_sequence.end())
			return boost::none;
		else
			return boost::fusion::at_c < 0 > (*seq_iter);
	}
}


void flat_playlist::mark_backend_resource_incompatibility(uri const &uri_, std::string const &backend_type)
{
}


uint64_t flat_playlist::get_num_entries() const
{
	return entries.size();
}


flat_playlist::entry_t const * flat_playlist::get_entry(uint64_t const nr) const
{
	if (nr >= get_num_entries())
		return 0;

	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator iter = entry_sequence.begin();
	iter = iter + nr;
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
	entries_by_uri_t const &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::const_iterator uri_iter = get_uri_iterator_for(uri_);
	if (uri_iter == entries_by_uri.end())
		return boost::none;

	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	entry_sequence_t::const_iterator sequence_iter = entries.project < sequence_tag > (uri_iter);
	return uint64_t(sequence_iter - entry_sequence.begin());
}


flat_playlist::entry_range_t flat_playlist::get_entry_range() const
{
	entry_sequence_t const &entry_sequence = entries.get < sequence_tag > ();
	return entry_range_t(entry_sequence.begin(), entry_sequence.end());
}


void flat_playlist::add_entry(entry_t entry_, bool const emit_signal)
{
	ion::uri &uri_ = boost::fusion::at_c < 0 > (entry_);

	unique_id_optional_t old_id = get_uri_id(uri_);
	if (old_id)
		unique_ids_.insert(*old_id);
	else
		set_uri_id(uri_, unique_ids_.create_new(), false);

	entries.push_back(entry_);
	if (emit_signal)
		resource_added_signal(boost::assign::list_of(uri_));
}


void flat_playlist::remove_entry(entry_t const &entry_, bool const emit_signal)
{
	ion::uri const &uri_ = boost::fusion::at_c < 0 > (entry_);

	unique_id_optional_t id = get_uri_id(uri_);
	if (id)
		unique_ids_.erase(*id);

	remove_entry(uri_, emit_signal);
}


void flat_playlist::remove_entry(uri const &uri_, bool const emit_signal)
{
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	// NOTE: erase must happen before the signal is emitted, because otherwise a transition event may be handled incorrectly (it may still see the entry)
	entries_by_uri.erase(uri_tag_iter);

	if (emit_signal)
		resource_removed_signal(boost::assign::list_of(uri_));
}


void flat_playlist::set_resource_metadata(uri const &uri_, metadata_t const & new_metadata)
{
	entries_by_uri_t &entries_by_uri = entries.get < uri_tag > ();
	entries_by_uri_t::iterator uri_tag_iter = get_uri_iterator_for(uri_);

	if (uri_tag_iter == entries_by_uri.end())
		return;

	entry_t entry_ = *uri_tag_iter;

	entries_by_uri.erase(uri_tag_iter);

	boost::fusion::at_c < 1 > (entry_) = new_metadata;
	entries.push_back(entry_);

	resource_metadata_changed_signal(boost::assign::list_of(uri_));
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


}

