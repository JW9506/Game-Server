using System;

public class tcp_packer {
  public static byte[] pack(byte[] cmd_data) {
    int len = cmd_data.Length;
    if (len > 65535 - 2)
    {
      return null;
    }
    byte[] res = new byte[len + 2];
    data_viewer.write_ushort_le(res, 0, (ushort)len);
    data_viewer.write_bytes(res, 2, cmd_data);
    return res;
  }
}
