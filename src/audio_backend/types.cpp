#include "types.hpp"


namespace ion
{
namespace audio_backend
{


unsigned int get_sample_size(sample_type const &type)
{
	switch (type)
	{
		case sample_s16: return 2;
		case sample_s24: return 3;
		default: return 0;
	}
}


}
}

