

/// packet_stream provides a network interface to the bit_stream for easy construction of data packets.
class packet_stream : public bit_stream
{
   typedef bit_stream Parent;
   uint8 buffer[udp_socket::max_datagram_size]; ///< internal buffer for packet data, sized to the maximum UDP packet size.
public:
   /// Constructor assigns the internal buffer to the bit_stream.
   packet_stream(uint32 target_packet_size = udp_socket::max_datagram_size) : bit_stream(buffer, udp_socket::max_datagram_size) {}
   /// Sends this packet to the specified address through the specified socket.
   udp_socket::send_to_result send_to(udp_socket &outgoing_socket, const address &the_address)
	{
	   return outgoing_socket.send_to(the_address, buffer, get_byte_position());
	}

   /// Reads a packet into the stream from the specified socket.
   udp_socket::recv_from_result recv_from(udp_socket &incomingSocket, address *recvAddress)
	{
	   udp_socket::recv_from_result the_result;
	   uint32 data_size;
	   the_result = incomingSocket.recv_from(recvAddress, buffer, sizeof(buffer), &data_size);
	   set_buffer(buffer, 0, data_size * 8);
	   return the_result;
	}

};
