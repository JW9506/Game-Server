using System;

public class data_viewer
{
  public static void write_ushort_le(byte[] buf, int offset, ushort value)
  {
    var byte_value = BitConverter.GetBytes(value);
    if (!BitConverter.IsLittleEndian)
    {
      Array.Reverse(byte_value);
    }
    Array.Copy(byte_value, 0, buf, offset, byte_value.Length);
  }
  public static void write_uint_le(byte[] buf, int offset, int value)
  {
    var byte_value = BitConverter.GetBytes(value);
    if (!BitConverter.IsLittleEndian)
    {
      Array.Reverse(byte_value);
    }
    Array.Copy(byte_value, 0, buf, offset, byte_value.Length);
  }
  public static void write_bytes(byte[] dst, int offset, byte[] value)
  {
    Array.Copy(value, 0, dst, offset, value.Length);
  }
}
