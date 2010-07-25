#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include "backend_handler.hpp"
#include "simple_playlist.hpp"


namespace ion
{
namespace frontend
{


simple_playlist::uri_optional_t simple_playlist::get_succeeding_uri(uri const &uri_) const
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


void simple_playlist::add_to_backend_handler(backend_handler &backend_handler_)
{
	backend_handler_.set_playlist(*this);
	resource_added_signal.connect(boost::lambda::bind(&backend_handler::resource_added, &backend_handler_, boost::lambda::_1));
	resource_removed_signal.connect(boost::lambda::bind(&backend_handler::resource_removed, &backend_handler_, boost::lambda::_1));
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

	entries_by_uri.erase(uri_tag_iter);

	resource_removed_signal(uri_);
}


}
}

