/// Encode the buffer to base 64, returning the encoded buffer.
static ref_ptr<byte_buffer> buffer_encode_base_64(const uint8 *buffer, uint32 buffer_size)
{
   unsigned long out_len = ((buffer_size / 3) + 1) * 4 + 4 + 1;
   byte_buffer *ret = new byte_buffer(out_len);
   base64_encode(buffer, buffer_size, ret->get_buffer(), &out_len);
   ret->resize(out_len+1);
   ret->get_buffer()[out_len] = 0;
   return ret;
}

/// Encode the buffer to base 64, returning the encoded buffer.
static inline ref_ptr<byte_buffer> buffer_encode_base_64(const byte_buffer &buffer)
{
   return buffer_encode_base_64(buffer.get_buffer(), buffer.get_buffer_size());
}

/// Decode the buffer from base 64, returning the decoded buffer.
static ref_ptr<byte_buffer> buffer_decode_base_64(const uint8 *buffer, uint32 buffer_size)
{
   unsigned long out_len = buffer_size;
   byte_buffer *ret = new byte_buffer(out_len);
   base64_decode(buffer, buffer_size, ret->get_buffer(), &out_len);
   ret->resize(out_len);
   return ret;
}


/// Decode the buffer from base 64, returning the decoded buffer.
static inline ref_ptr<byte_buffer> buffer_decode_base_64(const byte_buffer &buffer)
{
   return buffer_decode_base_64(buffer.get_buffer(), buffer.get_buffer_size());
}

/// Computes an MD5 hash and returns it in a byte_buffer
static ref_ptr<byte_buffer> buffer_compute_md5_hash(const uint8 *buffer, uint32 len)
{
   byte_buffer *ret = new byte_buffer(16);
   hash_state md;
   md5_init(&md);
   md5_process(&md, (unsigned char *) buffer, len);
   md5_done(&md, ret->get_buffer());
   return ret;
}

/// Converts to ascii-hex, returning the encoded buffer.
static ref_ptr<byte_buffer> buffer_encode_base_16(const uint8 *buffer, uint32 len)
{
   uint32 out_len = len * 2 + 1;
   byte_buffer *ret = new byte_buffer(out_len);
   uint8 *out_buffer = ret->get_buffer();

   for(uint32 i = 0; i < len; i++)
   {
      uint8 b = *buffer++;
      uint32 nib1 = b >> 4;
      uint32 nib2 = b & 0xF;
      if(nib1 > 9)
         *out_buffer++ = 'a' + nib1 - 10;
      else
         *out_buffer++ = '0' + nib1;
      if(nib2 > 9)
         *out_buffer++ = 'a' + nib2 - 10;
      else
         *out_buffer++ = '0' + nib2;
   }
   *out_buffer = 0;
   return ret;
}

/// Decodes the buffer from base 16, returning the decoded buffer.
static ref_ptr<byte_buffer> buffer_decode_base_16(const uint8 *buffer, uint32 len)
{
   uint32 out_len = len >> 1;
   byte_buffer *ret = new byte_buffer(out_len);
   uint8 *dst = ret->get_buffer();
   for(uint32 i = 0; i < out_len; i++)
   {
      uint8 out = 0;
      uint8 nib1 = *buffer++;
      uint8 nib2 = *buffer++;
      if(nib1 >= '0' && nib1 <= '9')
         out = (nib1 - '0') << 4;
      else if(nib1 >= 'a' && nib1 <= 'f')
         out = (nib1 - 'a' + 10) << 4;
      else if(nib1 >= 'A' && nib1 <= 'A')
         out = (nib1 - 'A' + 10) << 4;
      if(nib2 >= '0' && nib2 <= '9')
         out |= nib2 - '0';
      else if(nib2 >= 'a' && nib2 <= 'f')
         out |= nib2 - 'a' + 10;
      else if(nib2 >= 'A' && nib2 <= 'A')
         out |= nib2 - 'A' + 10;
      *dst++ = out;
   }
   return ret;
}


/// Returns a 32 bit CRC for the buffer.
static uint32 buffer_calculate_crc(const uint8 *buffer, uint32 len, uint32 crcVal = 0xFFFFFFFF)
{
   static uint32 crc_table[256];
   static bool crc_table_valid = false;

   if(!crc_table_valid)
   {
      uint32 val;

      for(int32 i = 0; i < 256; i++)
      {
         val = i;
         for(int32 j = 0; j < 8; j++)
         {
            if(val & 0x01)
               val = 0xedb88320 ^ (val >> 1);
            else
               val = val >> 1;
         }
         crc_table[i] = val;
      }
      crc_table_valid = true;
   }

   // now calculate the crc
   for(uint32 i = 0; i < len; i++)
      crcVal = crc_table[(crcVal ^ buffer[i]) & 0xff] ^ (crcVal >> 8);
   return(crcVal);
}

static void write_uint32_to_buffer(uint32 value, uint8 *buffer)
{
	buffer[0] = value >> 24;
	buffer[1] = value >> 16;
	buffer[2] = value >> 8;
	buffer[3] = value;
}

static uint32 read_uint32_from_buffer(const uint8 *buf)
{
	return (uint32(buf[0]) << 24) |
	(uint32(buf[1]) << 16) |
	(uint32(buf[2]) << 8 ) |
	uint32(buf[3]);
}

static void write_uint16_to_buffer(uint16 value, uint8 *buffer)
{
	buffer[0] = value >> 8;
	buffer[1] = (uint8) value;
}

static uint16 read_uint16_from_buffer(const uint8 *buffer)
{
	return (uint16(buffer[0]) << 8) |
	uint16(buffer[1]);
}


