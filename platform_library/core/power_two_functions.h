/// Determines if number is a power of two.
inline bool is_power_of_2(const uint32 number)
{
	return (number & (number - 1)) == 0;
}

/// Determines the binary logarithm of the input value rounded down to the nearest power of 2.
inline uint32 get_binary_log(uint32 value)
{
	float32 floatValue = float32(value);
	return (*((uint32 *) &floatValue) >> 23) - 127;
}

/// Determines the binary logarithm of the next greater power of two of the input number.
inline uint32 get_next_binary_log(uint32 number)
{
	return get_binary_log(number) + (is_power_of_2(number) ? 0 : 1);
}

/// Determines the next greater power of two from the value.  If the value is a power of two, it is returned.
inline uint32 get_next_power_of_2(uint32 value)
{
	return is_power_of_2(value) ? value : (1 << (get_binary_log(value) + 1));
}

