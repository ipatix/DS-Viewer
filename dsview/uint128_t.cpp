#include "uint128_t.h"
#include <cstdio>

uint128_t::uint128_t(uint64_t val)
{
	low = val;
	high = 0;
}

uint128_t::uint128_t(uint64_t low, uint64_t high)
{
	this->low = low;
	this->high = high;
}

uint128_t uint128_t::operator << (const uint128_t & val) const
{
	if (val.high > 0) return uint128_t(0, 0);
	uint64_t new_low = (val.low < 64) ? low << val.low : 0;
	uint64_t new_high = (val.low < 64) ? (high << val.low) : 0;
	if (val.low < 64)
		new_high |= (low >> (64 - val.low));
	else
		new_high |= (val.low - 64 < 64) ? (low << (val.low - 64)) : 0;
	return uint128_t(new_low, new_high);
}

uint128_t uint128_t::operator >> (const uint128_t & val) const
{
	if (val.high > 0) return uint128_t(0, 0);
	uint64_t new_low = (val.low < 64) ? (low >> val.low) : 0;
	if (val.low < 64)
		new_low |= (high << (64 - val.low));
	else
		new_low |= (val.low - 64 < 64) ? (high >> (val.low - 64)) : 0;
	uint64_t new_high = (val.low < 64) ? high >> val.low : 0;
	return uint128_t(new_low, new_high);
}

uint128_t uint128_t::operator & (const uint128_t & val) const
{
	return uint128_t(low & val.low, high & val.high);
}

uint128_t uint128_t::operator | (const uint128_t & val) const
{
	return uint128_t(low | val.low, high | val.high);
}

uint128_t & uint128_t::operator<<=(const uint128_t & val)
{
	if (val.high > 0)
	{
		low = high = 0;
		return *this;
	}
	uint64_t new_low = (val.low < 64) ? low << val.low : 0;
	uint64_t new_high = (val.low < 64) ? (high << val.low) : 0;
	if (val.low < 64)
		new_high |= (low >> (64 - val.low));
	else
		new_high |= (val.low - 64 < 64) ? (low << (val.low - 64)) : 0;
	low = new_low;
	high = new_high;
	return *this;
}

uint128_t & uint128_t::operator >>= (const uint128_t & val)
{
	if (val.high > 0)
	{
		low = high = 0;
		return *this;
	}
	uint64_t new_low = (val.low < 64) ? (low >> val.low) : 0;
	if (val.low < 64)
		new_low |= (high << (64 - val.low));
	else
		new_low |= (val.low - 64 < 64) ? (high >> (val.low - 64)) : 0;
	uint64_t new_high = (val.low < 64) ? high >> val.low : 0;
	low = new_low;
	high = new_high;
	return *this;
}

uint128_t & uint128_t::operator&=(const uint128_t & val)
{
	low &= val.low;
	high &= val.high;
	return *this;
}

uint128_t & uint128_t::operator|=(const uint128_t & val)
{
	low |= val.low;
	high |= val.high;
	return *this;
}

uint128_t & uint128_t::operator = (const uint128_t & val)
{
	low = val.low;
	high = val.high;
	return *this;
}

bool uint128_t::operator==(const uint128_t & val) const
{
	return low == val.low && high == val.high;
}

uint128_t::operator bool() const
{
	return (low & 1) ? true : false;
}

uint128_t::operator char() const
{
	return char(low);
}

uint128_t::operator int() const
{
	return int(low);
}

uint128_t::operator uint8_t() const
{
	return uint8_t(low);
}

uint128_t::operator uint16_t() const
{
	return uint16_t(low);
}

uint128_t::operator uint32_t() const
{
	return uint32_t(low);
}

uint128_t::operator uint64_t() const
{
	return low;
}

void uint128_t::print()
{
	printf("%lX-%lX\n", high, low);
}
