

/// packet_stream provides a network interface to the bit_stream for easy construction of data packets.
class packet_stream : public bit_stream
{
   typedef bit_stream Parent;
   uint8 buffer[udp_socket::max_datagram_size]; ///< internal buffer for packet data, sized to the maximum UDP packet size.
public:
   /// Constructor assigns the internal buffer to the bit_stream.
   packet_stream(uint32 target_packet_size = udp_socket::max_datagram_size) : bit_stream(buffer, udp_socket::max_datagram_size) {}
	packet_stream(const packet_stream& other) : bit_stream(other)
	{
		memcpy(buffer, other.buffer, sizeof(buffer));
	}
	
	string to_string()
	{
		byte_buffer_ptr b = buffer_encode_base_16(get_buffer(), get_next_byte_position());
		return string((const char *) b->get_buffer());
	}
	
	packet_stream& operator=(const packet_stream& other)
	{
		memcpy(buffer, other.buffer, sizeof(buffer));
	}
	
	void set_from_buffer(const uint8 *in_buffer, uint32 size)
	{
		assert(size <= udp_socket::max_datagram_size);
		memcpy(buffer, in_buffer, size);
		set_buffer(buffer, 0, size * 8);
	}
	
   /// Sends this packet to the specified address through the specified socket.
   udp_socket::send_to_result send_to(udp_socket &outgoing_socket, const address &the_address)
	{
		return outgoing_socket.send_to(the_address, buffer, get_next_byte_position());
	}

   /// Reads a packet into the stream from the specified socket.
   udp_socket::recv_from_result recv_from(udp_socket &incoming_socket, address *recv_address)
	{
	   udp_socket::recv_from_result the_result;
	   uint32 data_size;
	   the_result = incoming_socket.recv_from(recv_address, buffer, sizeof(buffer), &data_size);
		logprintf("packet read, datasize = %d", data_size);
	   set_buffer(buffer, 0, data_size * 8);
	   return the_result;
	}

};
