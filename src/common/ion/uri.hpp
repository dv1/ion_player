#ifndef ION_URI_HPP
#define ION_URI_HPP

#include <iostream>
#include <stdexcept>
#include <map>
#include <string>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>


namespace ion
{


class uri
{
public:
	class invalid_uri:
		public std::runtime_error
	{
	public:
		explicit invalid_uri(std::string const &erroneous_uri):
			runtime_error(erroneous_uri.c_str())
		{
		}
	};


	typedef std::map < std::string, std::string > options_t;


	uri()
	{
	}

	uri(std::string const &full_uri)
	{
		set(full_uri);
	}


	void set(std::string const &full_uri)
	{
		std::size_t delimiter_pos = full_uri.find("://");

		if ((delimiter_pos > 0) && ((int(full_uri.length()) - int(delimiter_pos) - 3) > 0) && (delimiter_pos != std::string::npos))
		{
			std::string new_type = full_uri.substr(0, delimiter_pos);
			std::string new_path = full_uri.substr(std::streamoff(delimiter_pos) + 3);

			if (new_type.empty()) std::cerr << "warning: uri \"" << full_uri << "\" type is empty";
			if (new_path.empty()) std::cerr << "warning: uri \"" << full_uri << "\" path is empty";

			{
				std::size_t options_delimiter_pos = new_path.find_first_of("?");
				if (options_delimiter_pos != std::string::npos)
				{
					std::string path_options = new_path.substr(options_delimiter_pos + 1);
					new_path = new_path.substr(0, options_delimiter_pos);

					options_t new_options;
					std::string current_option;
					bool escaping = false;

					BOOST_FOREACH(char c, path_options)
					{
						if (escaping)
						{
							switch (c)
							{
								case '\\': current_option += '\\'; break;
								case '?': current_option += '?'; break;
								case '&': current_option += '&'; break;
								case '=': current_option += "\\="; break; // pass the escaped char unchanged, so add_option() can handle it itself
								default: throw invalid_uri(full_uri);
							}
							escaping = false;
						}
						else
						{
							switch (c)
							{
								case '\\': escaping = true; break;

								case '&':
									if (!current_option.empty())
									{
										add_to_options(new_options, current_option, full_uri);
										current_option = "";
									}
									break;

								default: current_option += c;
							}
						}
					}

					if (escaping)
						throw invalid_uri(full_uri); // if still escaping after the uri was scanned, then there is a "\" at the end of it -> invalid

					if (!current_option.empty())
					{
						add_to_options(new_options, current_option, full_uri);
					}

					options = new_options;
				}
			}

			type = new_type;
			path = new_path;
		}
		else
			throw invalid_uri(full_uri);
	}


	uri& operator = (std::string const &full_uri)
	{
		set(full_uri);
		return *this;
	}


	bool operator == (uri const &other) const
	{
		return (type == other.type) && (path == other.path) && (options == other.options);
	}

	bool operator != (uri const &other) const
	{
		return ! (operator == (other));
	}

	bool operator < (uri const &other) const
	{
		// this test takes equality into account, using the type, path, options values.
		// If value < other.value, then return true, since this uri is definitely less than the other one.
		// If value > other.value, then return false, since this uri is definitely greater than the other one.
		// Otherwise, the values in question are equal, so move to the next value.
		// If all three values are equal, return false - x < x is always false, after all.
		// This is sort of a lexicographic compare: for instance, an abc < abd comparison would first see that the first letters are both a,
		// then continue to the second letters, which are both b, and finally the third letters are c and d, where c<d holds -> return true.

		if (type < other.type)
			return true;
		else if (type > other.type)
			return false;

		if (path < other.path)
			return true;
		else if (path > other.path)
			return false;

		else if (options < other.options)
			return true;
		else if (options > other.options)
			return false;

		return false;
	}


	std::string const & get_type() const { return type; }
	std::string const & get_path() const { return path; }
	options_t const & get_options() const { return options; }
	options_t & get_options() { return options; }

	std::string get_full() const
	{
		std::string result = type + "://" + path;

		for (options_t::const_iterator option_iter = options.begin(); option_iter != options.end(); ++option_iter)
		{
			result += (option_iter == options.begin()) ? '?' : '&';
			result += option_iter->first + '=' + option_iter->second;
		}

		return result;
	//	return type + "://" + path;
	}


protected:
	void add_to_options(options_t &options_, std::string const &option, std::string const &full_uri)
	{
		std::string key, value;

		bool has_value = false, escaping = false;
		BOOST_FOREACH(char c, option)
		{
			std::string &dest = has_value ? value : key;

			if (escaping)
			{
				switch (c)
				{
					case '=': dest += '='; break;
					default: throw invalid_uri(full_uri);
				}
				escaping = false;
			}
			else
			{
				switch (c)
				{
					case '\\': escaping = true; break;
					case '=':
						if (!has_value)
							has_value = true;
						else
							throw invalid_uri(full_uri);
						break;
					default: dest += c; break;
				}
			}
		}

		options_[key] = value;
	}


	std::string type, path;
	options_t options;
};


typedef boost::optional < uri > uri_optional_t;


}


#endif

