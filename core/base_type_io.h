// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

template <class stream, typename type> inline bool write(stream &the_stream, const type &value)
{
	type temp = value;
	host_to_little_endian(temp);
	return the_stream.write_bytes((byte *) &temp, sizeof(temp)) == sizeof(temp);
}

template <class stream, typename type> inline bool read(stream &the_stream, type &value)
{
	if(the_stream.read_bytes((byte *) &value, sizeof(type)) != sizeof(type))
		return false;

	host_to_little_endian(value);
	return true;
}

template <class stream, uint32 enum_count> inline bool write(stream &the_stream, const enumeration<enum_count> &value)
{
	the_stream.write_ranged_uint32(value, 0, enum_count - 1);
	return true;
}

template <class stream, uint32 enum_count> inline bool read(stream &the_stream, enumeration<enum_count> &value)
{
	value = the_stream.read_ranged_uint32(0, enum_count - 1);
	return true;
}

template <class stream, int32 range_min, int32 range_max> inline bool write(stream &the_stream, const int_ranged<range_min, range_max> &value)
{
	the_stream.write_ranged_uint32(value.offset_value, 0, range_max - range_min + 1);
	return true;
}

template <class stream, int32 range_min, int32 range_max> inline bool read(stream &the_stream,  int_ranged<range_min, range_max> &value)
{
	value.offset_value = the_stream.read_ranged_uint32(0, range_max - range_min + 1);
	return true;
}

template <class stream, uint32 precision> inline bool write(stream &the_stream, const unit_float<precision> &value)
{
	the_stream.write_integer(uint32(value.value * ((1 << precision) - 1)), precision);
	return true;
}

template <class stream, uint32 precision> inline bool read(stream &the_stream, unit_float<precision> &value)
{
	value.value = the_stream.read_integer(precision) / float32((1 << precision) - 1);
	return true;
}

template <class stream, uint32 precision> inline bool write(stream &the_stream, const signed_unit_float<precision> &value)
{
	the_stream.write_integer(uint32(( (value.value + 1)*.5 ) * ((1 << precision) - 1) ), precision);
	return true;
}

template <class stream, uint32 precision> inline bool read(stream &the_stream, signed_unit_float<precision> &value)
{
	value.value = (the_stream.read_integer(precision) / float32((1 << precision) - 1)) * 2.0f - 1.0f;
	return true;
}
