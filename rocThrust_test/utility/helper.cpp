#include "helper.hpp"

void dim_gen(const int a, const int b, std::vector<int>& dim, const int blocks)
{
	if(blocks == 0)
	{
		dim.resize(1);
		dim[0] = b;
		return;
	}
	dim.resize(blocks);
	int blk_size = (b - a) / blocks;
	int ct = blocks;
	while(ct)
	{
		dim[blocks - ct] = a + blk_size*(blocks - ct);
		ct--;
	}
}
