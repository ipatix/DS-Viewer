#pragma once

#include <cstdint>

class uint128_t
{
public:
	uint128_t(uint64_t val);
	uint128_t(uint64_t low, uint64_t high);

	uint128_t operator << (const uint128_t& val) const;
	uint128_t operator >> (const uint128_t& val) const;
	uint128_t operator & (const uint128_t& val) const;
	uint128_t operator | (const uint128_t& val) const;

	uint128_t& operator <<= (const uint128_t& val);
	uint128_t& operator >>= (const uint128_t& val);
	uint128_t& operator &= (const uint128_t& val);
	uint128_t& operator |= (const uint128_t& val);

	uint128_t& operator=(const uint128_t& val);
	bool operator==(const uint128_t& val) const;

	operator bool() const;
	operator char() const;
	operator int() const;
	operator uint8_t() const;
	operator uint16_t() const;
	operator uint32_t() const;
	operator uint64_t() const;

	void print();

private:
	uint64_t low, high;
};