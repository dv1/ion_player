#include "playlist.hpp"


namespace ion
{
namespace frontend
{


playlist::playlist()
{
}


playlist::~playlist()
{
}


void playlist::transition()
{
}


void playlist::play(resource_id_t const &resource_id)
{
}


playlist::resource_id_optional_t playlist::get_entry_id(uint64_t const nr)
{
	if (nr >= get_num_entries())
		return boost::none;

	typedef playlist_entries_t::index < by_index_tag > ::type playlist_entries_by_index_t;
	playlist_entries_by_index_t &playlist_entries_by_index = playlist_entries.get < by_index_tag > ();
	return (playlist_entries_by_index.begin() + nr)->resource_id;
}


uint64_t playlist::get_num_entries()
{
	return playlist_entries.size();
}


playlist::playlist_entry_optional_t playlist::get_entry(resource_id_t const &resource_id)
{
	typedef playlist_entries_t::index < by_resource_id_tag > ::type playlist_entries_by_id_t;
	playlist_entries_by_id_t &playlist_entries_by_id = playlist_entries.get < by_resource_id_tag > ();
	playlist_entries_by_id_t::iterator iter = playlist_entries_by_id.find(resource_id);

	if (iter != playlist_entries_by_id.end())
		return *iter;
	else
		return boost::none;
}


}
}

