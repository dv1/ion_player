#ifndef ION_URI_HPP
#define ION_URI_HPP

#include <iostream>
#include <stdexcept>
#include <set>
#include <map>
#include <string>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>


namespace ion
{


/*
This class parses and stores URIs. A URI (Uniform Resource Identifier) is the way Ion points to a resource and passes options to it in a persistent manner.
The format of a URI is:    <type>://<path>?<param_name_1>=<param_value_1>&<param_name_2>=<param_value_2>&....
Note that this URI format is not the same as what is named "URI" by the W3C.

type and path must exist in a URI. The options are optional. The ? symbol denotes the ending of the path and beginning of the options list. If no options exist in the URI,
the ? symbol is omitted.

The URI can store a list of options. Each option has a name and a value.
The options are separated with the & symbol; the last option omits the trailing & symbol.

Examples:

sometype://somepath -> type = "sometype", path = "somepath", no options
sometype://somepath&a=b -> type = "sometype", path = "somepath", one option "a" with value "b"
sometype://somepath&a=b&c=d -> type = "sometype", path = "somepath", two options, one option "a" with value "b", and a second option "c" with value "d"
sometype://somepath&a=b&c=d&e=f -> type = "sometype", path = "somepath", three options

The symbols in option names and values are escaped if necessary.
*/


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

	// Convenience constructor calling set() internally
	uri(std::string const &full_uri)
	{
		set(full_uri);
	}


	// Sets the URI. The given string gets parsed. If the string does not contain a valid URI, an invalid_uri exception is thrown.
	//
	// @param full_uri A string to be parsed, containing a full URI
	// @post This class' states will be set to the ones described in the URI string if parsing was successful; if the string is found to be invalid,
	// an invalid_uri exception is thrown, and no state is changed
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
		/*
		this test takes equality into account, using the type, path, options values.
		If value < other.value, then return true, since this uri is definitely less than the other one.
		If value > other.value, then return false, since this uri is definitely greater than the other one.
		Otherwise, the values in question are equal, so move to the next value.
		If all three values are equal, return false - x < x is always false, after all.
		This is sort of a lexicographic compare: for instance, an abc < abd comparison would first see that the first letters are both a,
		then continue to the second letters, which are both b, and finally the third letters are c and d, where c<d holds -> return true.
		*/

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


	// Returns the URI type
	std::string const & get_type() const
	{
		return type;
	}


	// Returns the URI path
	std::string const & get_path() const
	{
		return path;
	}


	// Returns the URI options as a STL map, key = option name, value = option value
	options_t const & get_options() const
	{
		return options;
	}


	// Returns the URI options as a STL map, key = option name, value = option value
	options_t & get_options()
	{
		return options;
	}


	// Returns the URI basename. The basename is the last segment of a path. For instance, in a filepath, this corresponds to the filename without path.
	// Example: sometype://this/is/a/path/to/something?a=b -> basename is "something"
	std::string get_basename() const
	{
		std::string::size_type slash_pos = path.find_last_of('/');
		if (slash_pos != std::string::npos)
			return path.substr(slash_pos + 1);
		else
			return path;
	}


	// Reconstruct a full URI string from the internal states
	// @return A full URI string describing the resource
	std::string get_full() const
	{
		std::string result = type + "://" + path;

		for (options_t::const_iterator option_iter = options.begin(); option_iter != options.end(); ++option_iter)
		{
			result += (option_iter == options.begin()) ? '?' : '&';
			result += option_iter->first + '=' + option_iter->second;
		}

		return result;
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
typedef std::set < ion::uri > uri_set_t;


}


#endif

