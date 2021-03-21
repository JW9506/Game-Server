using System;

public class tcp_packer
{
  private const int HEADER_SIZE = 2;
  public static byte[] pack(byte[] cmd_data)
  {
    int len = cmd_data.Length;
    if (len > 65535 - HEADER_SIZE)
    {
      return null;
    }
    byte[] res = new byte[len + HEADER_SIZE];
    data_viewer.write_ushort_le(res, 0, (ushort)len);
    data_viewer.write_bytes(res, HEADER_SIZE, cmd_data);
    return res;
  }
  public static bool read_header(byte[] data, int data_len, out int pkg_size, out int head_size)
  {
    pkg_size = 0;
    head_size = 0;
    if (data_len < 2)
    {
      return false;
    }
    pkg_size = data_viewer.read_ushort_le(data, 0);
    head_size = HEADER_SIZE;
    return true;
  }
}
