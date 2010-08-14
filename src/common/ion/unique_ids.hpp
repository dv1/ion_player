#ifndef ION_UNIQUE_IDS_HPP
#define ION_UNIQUE_IDS_HPP

#include <cstdlib>
#include <tr1/unordered_set>


namespace ion
{


/*
This class generates and stores unique IDs. The uniqueness of the IDs is guaranteed.
It works by generating a random number until said number is found to be unused (that is, it is not present in the internal ID list).
This number is then stored in the ID list and returned. Once the ID is not used, it is removed from the internal ID list.

The randomness aspect avoids a linear increase in time and does not cause "holes". For instance, if one always starts at zero and counts up until an unused number
is found, then the amount of increments depends on the amount of already used IDs. Starting at the current maximum ID may introduce "holes" if a smaller ID was
removed from the list before.
*/

template < typename Id >
class unique_ids
{
protected:
	typedef std::tr1::unordered_set < Id > ids_t;


public:
	typedef Id id_t;


	/*
	Creates a new unique ID as described above.
	@return The new unique ID
	@post The internal ID list will be increased in size by one entry - the new ID
	*/
	Id create_new()
	{
		Id id;
		typename ids_t::iterator iter;
		do
		{
			id = Id(std::rand()); // TODO: use Id's numeric range, instead of just 0 - RAND_MAX
			iter = ids.find(id);
		}
		while (iter != ids.end());

		ids.insert(id);

		return id;
	}


	/*
	Insert an ID into the internal ID list. This function does _not_ create an ID. It is used to manually mark certain IDs as used.
	@param id The id to store in the internal list, thereby marking it as used
	@post The internal list will be increased in size by one entry (the id) if it wasnt already included; if it was, nothing changed
	*/
	void insert(Id const &id)
	{
		ids.insert(id);
	}


	/*
	Removes an ID from the internal list, thereby marking it as unused and available.
	@param id The id to remove from internal list, thereby marking it as unused
	@post The internal list will be decreased in size by one entry (the id) if it was included in the list; if it wasn't, nothing changed
	*/
	void erase(Id const &id)
	{
		typename ids_t::iterator iter = ids.find(id);
		if (iter != ids.end())
			ids.erase(iter);
	}


	// Return the internal list of used IDs
	ids_t const & get_used_ids() const
	{
		return ids;
	}


protected:
	ids_t ids;
};


}


#endif

