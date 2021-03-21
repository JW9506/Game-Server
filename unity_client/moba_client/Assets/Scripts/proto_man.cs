using System;
using System.IO;
using System.Text;
using ProtoBuf;
using UnityEngine;

public class cmd_msg {
  public int stype;
  public int ctype;
  public byte[] body;
}
public class proto_man
{
  private const int HEADER_SIZE = 8;
  private static byte[] protobuf_serializer(ProtoBuf.IExtensible data) {
    using (MemoryStream m = new MemoryStream())
    {
      byte[] buffer = null;
      Serializer.Serialize(m, data);
      m.Position = 0;
      int length = (int)m.Length;
      buffer = new byte[length];
      m.Read(buffer, 0, length);
      return buffer;
    }
  }
  public static byte[] pack_protobuf_cmd(int stype, int ctype, ProtoBuf.IExtensible msg)
  {
    int cmd_len = HEADER_SIZE;
    byte[] cmd_body = null;
    if (msg != null)
    {
      cmd_body = protobuf_serializer(msg);
      cmd_len += cmd_body.Length;
    }
    byte[] cmd = new byte[cmd_len];
    data_viewer.write_ushort_le(cmd, 0, (ushort)stype);
    data_viewer.write_ushort_le(cmd, 2, (ushort)ctype);
    if (cmd_body != null) {
      data_viewer.write_bytes(cmd, HEADER_SIZE, cmd_body);
    }
    return cmd;
  }
  public static byte[] pack_json_cmd(int stype, int ctype, string json_msg)
  {
    int cmd_len = HEADER_SIZE;
    byte[] cmd_body = null;
    if (json_msg.Length > 0)
    {
      cmd_body = Encoding.UTF8.GetBytes(json_msg);
      cmd_len += cmd_body.Length;
    }
    byte[] cmd = new byte[cmd_len];
    data_viewer.write_ushort_le(cmd, 0, (ushort)stype);
    data_viewer.write_ushort_le(cmd, 2, (ushort)ctype);
    if (cmd_body != null) {
      data_viewer.write_bytes(cmd, HEADER_SIZE, cmd_body);
    }
    return cmd;
  }
  public static bool unpack_cmd_msg(
    byte[] data,
    int offset,
    int data_len,
    out cmd_msg msg)
  {
    msg = new cmd_msg();
    msg.stype = data_viewer.read_ushort_le(data, offset);
    msg.ctype = data_viewer.read_ushort_le(data, offset + 2);
    int body_len = data_len - HEADER_SIZE;
    msg.body = new byte[body_len];
    Array.Copy(data, offset + HEADER_SIZE, msg.body, 0, body_len);
    return true;
  }
  public static T protobuf_deserialize<T>(byte[] _data)
  {
    using (MemoryStream m = new MemoryStream(_data))
    {
      return Serializer.Deserialize<T>(m);
    }
  }
}
