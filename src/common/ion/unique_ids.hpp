#ifndef ION_UNIQUE_IDS_HPP
#define ION_UNIQUE_IDS_HPP

#include <cstdlib>
#include <tr1/unordered_set>


namespace ion
{


template < typename T >
class unique_ids
{
protected:
	typedef std::tr1::unordered_set < T > ids_t;


public:
	typedef T id_t;


	T create_new()
	{
		T id = T(std::rand()); // TODO: use the type T's range, instead of just 0 - RAND_MAX

		typename ids_t::iterator iter;
		do
		{
			iter = ids.find(id);
		}
		while (iter != ids.end());

		return id;
	}


	void insert(T const &id)
	{
		ids.insert(id);
	}


	void erase(T const &id)
	{
		typename ids_t::iterator iter = ids.find(id);
		if (iter != ids.end())
			ids.erase(iter);
	}


protected:
	ids_t ids;
};


}


#endif

