#include "test.hpp"
#include <boost/foreach.hpp>
#include <set>
#include <ion/uri.hpp>


int test_main(int, char **)
{
	{
		ion::uri uri_("http://localhost");
		TEST_ASSERT(uri_.get_type() == "http", "");
		TEST_ASSERT(uri_.get_path() == "localhost", "");
	}

	{
		TEST_EXCEPTION(ion::uri::invalid_uri, ion::uri("abc://"), "");
	}

	{
		TEST_EXCEPTION(ion::uri::invalid_uri, ion::uri("://def"), "");
	}

	{
		ion::uri uri_;
		TEST_NO_EXCEPTION(uri_.set("http://localhost"), "");
		TEST_EXCEPTION(ion::uri::invalid_uri, uri_.set("a://"), "");
		TEST_EXCEPTION(ion::uri::invalid_uri, uri_.set("://b"), "");
		TEST_EXCEPTION(ion::uri::invalid_uri, uri_.set("://"), "");
		TEST_ASSERT(uri_.get_type() == "http", "");
		TEST_ASSERT(uri_.get_path() == "localhost", "");
		TEST_ASSERT(uri_.get_basename() == "localhost", "");
	}

	{
		ion::uri uri_;
		TEST_NO_EXCEPTION(uri_.set("http://localhost/abc/def/ghi.json?xxx=yyy"), "");
		TEST_ASSERT(uri_.get_type() == "http", "");
		TEST_ASSERT(uri_.get_path() == "localhost/abc/def/ghi.json", "");
		TEST_ASSERT(uri_.get_basename() == "ghi.json", "");
	}

	{
		ion::uri uri_;
		TEST_NO_EXCEPTION(uri_.set("http://localhost?abc=5&&"), "");
		TEST_NO_EXCEPTION(uri_.set("http://localhost?&abc=5"), "");
		TEST_NO_EXCEPTION(uri_.set("http://localhost?abc=5&"), "");
		TEST_EXCEPTION(ion::uri::invalid_uri, uri_.set("http://localhost?abc=\\x"), "");
		TEST_EXCEPTION(ion::uri::invalid_uri, uri_.set("http://localhost?abc=\\"), "");

		TEST_NO_EXCEPTION(uri_.set("http://localhost?"), "");
		TEST_ASSERT(uri_.get_options().size() == 0, "got " << uri_.get_options().size());

		TEST_NO_EXCEPTION(uri_.set("http://localhost?abc=5"), "");
		TEST_ASSERT(uri_.get_options().size() == 1, "got " << uri_.get_options().size());

		TEST_NO_EXCEPTION(uri_.set("http://localhost?abc=5&def=2"), "");
		TEST_ASSERT(uri_.get_options().size() == 2, "got " << uri_.get_options().size());

		TEST_NO_EXCEPTION(uri_.set("http://localhost?&&&&&abc=5&&&&"), "");
		TEST_ASSERT(uri_.get_options().size() == 1, "got " << uri_.get_options().size());

		TEST_NO_EXCEPTION(uri_.set("http://localhost?&&&&&abc=5&&def=2&&"), "");
		TEST_ASSERT(uri_.get_options().size() == 2, "got " << uri_.get_options().size());
	}

	{
		ion::uri uri1, uri2, uri3, uri4;

		TEST_NO_EXCEPTION(uri1.set("http://localhost?abc=5"), "");
		TEST_NO_EXCEPTION(uri2.set("http://localhost?abc=4"), "");
		TEST_NO_EXCEPTION(uri3.set("http://xxx?abc=15124"), "");
		TEST_NO_EXCEPTION(uri4.set("http://localhost?abc=4"), "");
		TEST_ASSERT(!(uri1 < uri2), "");
		TEST_ASSERT(uri2 < uri1, "");
		TEST_ASSERT(uri1 < uri3, "");
		TEST_ASSERT(uri2 < uri3, "");
		TEST_ASSERT(uri2 == uri4, "");
	}

	{
		std::set < ion::uri > uris;
		uris.insert(ion::uri("http://localhost?abc=5"));
		uris.insert(ion::uri("http://localhost?abc=4"));
		uris.insert(ion::uri("http://xxx?abc=15124"));
		uris.insert(ion::uri("http://localhost?abc=4"));

		BOOST_FOREACH(ion::uri const &uri_, uris)
		{
			TEST_ASSERT(uris.find(uri_) != uris.end(), "Unable to find URI " << uri_.get_full());
		}
	}

	{
		ion::uri uri_;
		TEST_NO_EXCEPTION(uri_.set("http://localhost?abc=5&def=2"), "");
		TEST_ASSERT(uri_.get_options().size() == 2, "got " << uri_.get_options().size());
		TEST_ASSERT(uri_.get_path() == "localhost", "got " << uri_.get_path());
		TEST_ASSERT(uri_.get_full() == "http://localhost?abc=5&def=2", "got " << uri_.get_full());
	}

	{
		ion::uri uri_("http://localhost?abc=5&ugh=&ztt&def=i7x&x\\&=\\?y");
		TEST_ASSERT(uri_.get_type() == "http", "got " << uri_.get_type());
		TEST_ASSERT(uri_.get_path() == "localhost", "got " << uri_.get_path());
		TEST_ASSERT(uri_.get_options().size() == 5, "got " << uri_.get_options().size());
		TEST_ASSERT(uri_.get_options().find("abc") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("ugh") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("ztt") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("def") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("x&") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("abc")->second == "5", "got " << uri_.get_options().find("abc")->second);
		TEST_ASSERT(uri_.get_options().find("ugh")->second == "", "got " << uri_.get_options().find("ugh")->second);
		TEST_ASSERT(uri_.get_options().find("ztt")->second == "", "got " << uri_.get_options().find("ztt")->second);
		TEST_ASSERT(uri_.get_options().find("def")->second == "i7x", "got " << uri_.get_options().find("def")->second);
		TEST_ASSERT(uri_.get_options().find("x&")->second == "?y", "got " << uri_.get_options().find("x&")->second);
	}

	{
		ion::uri uri_("http://localhost?a\\=b=d\\=e");
		TEST_ASSERT(uri_.get_options().size() == 1, "got " << uri_.get_options().size());
		TEST_ASSERT(uri_.get_options().find("a=b") != uri_.get_options().end(), "");
		TEST_ASSERT(uri_.get_options().find("a=b")->second == "d=e", "got " << uri_.get_options().find("a=b")->second);
	}

	return 0;
}


INIT_TEST

