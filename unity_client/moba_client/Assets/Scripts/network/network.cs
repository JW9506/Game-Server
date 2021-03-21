using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class network : MonoBehaviour
{
  public string server_ip;
  public int port;
  public delegate void net_message_handle(cmd_msg cmd);
  public static network instance
  {
    get
    {
      return _instance;
    }
  }

  private static network _instance;
  private Socket client_socket = null;
  private bool is_connect = false;
  private Thread recv_thread = null;
  private const int RECV_LEN = 8192;
  private byte[] recv_buf = new byte[RECV_LEN];
  private int recved = 0;
  private byte[] long_pkg = null;
  private int long_pkg_size = 0;
  private Queue<cmd_msg> net_events = new Queue<cmd_msg>();
  private Dictionary<int, net_message_handle> event_listeners = new Dictionary<int, net_message_handle>();
  void Awake()
  {
    _instance = this;
    DontDestroyOnLoad(this.gameObject);
  }
  void Start()
  {
    connect_to_server();
  }
  void OnDestroy()
  {
    this.close();
  }
  void OnApplicationQuit()
  {
    this.close();
  }
  void Update()
  {
    lock (net_events)
    {
      while (net_events.Count > 0)
      {
        cmd_msg msg = net_events.Dequeue();
        if (event_listeners.ContainsKey(msg.stype))
        {
          event_listeners[msg.stype](msg);
        }
      }
    }
  }
  void connect_to_server()
  {
    try
    {
      client_socket = new Socket(AddressFamily.InterNetwork,
                                 SocketType.Stream,
                                 ProtocolType.Tcp);
      var ipAddress = IPAddress.Parse(this.server_ip);
      var ipEndPoint = new IPEndPoint(ipAddress, this.port);
      var result = client_socket.BeginConnect(
        ipEndPoint,
        new AsyncCallback(on_connect_success),
        client_socket);
      var timeout = result.AsyncWaitHandle.WaitOne(4000, true);
      if (!timeout)
      {
        on_connect_timeout();
      }
    }
    catch (System.Exception e)
    {
      on_connect_error(e.ToString());
    }
  }
  void on_connect_success(IAsyncResult iar)
  {
    try
    {
      var client = (Socket)iar.AsyncState;
      client.EndConnect(iar);
      is_connect = true;
      recv_thread = new Thread(new ThreadStart(recv_worker_thread));
      recv_thread.Start();
      Debug.Log("Connected to " + server_ip + ":" + port);
    }
    catch (System.Exception e)
    {
      on_connect_error(e.ToString());
      is_connect = false;
    }
  }
  void recv_worker_thread()
  {
    if (!this.is_connect)
    {
      return;
    }
    while (true)
    {
      if (!client_socket.Connected)
      {
        break;
      }
      try
      {
        int recv_len = 0;
        if (recved < RECV_LEN)
        {
          recv_len = client_socket.Receive(
            recv_buf,
            recved,
            RECV_LEN - recved,
            SocketFlags.None);
        }
        else
        {
          if (long_pkg == null)
          {
            int pkg_size;
            int head_size;
            tcp_packer.read_header(recv_buf, recved, out pkg_size, out head_size);
            long_pkg_size = pkg_size;
            long_pkg = new byte[pkg_size];
            Array.Copy(recv_buf, 0, long_pkg, 0, recved);
          }
          recv_len = client_socket.Receive(
            long_pkg,
            recved,
            long_pkg_size - recved,
            SocketFlags.None);
        }
        if (recv_len > 0)
        {
          recved += recv_len;
          on_recv_tcp_data();
        }
      }
      catch (System.Exception)
      {
        if (client_socket != null && client_socket.Connected)
        {
          client_socket.Disconnect(true);
          client_socket.Shutdown(SocketShutdown.Both);
          client_socket.Close();
        }
        is_connect = false;
        break;
      }
    }
  }
  void on_recv_tcp_data()
  {
    byte[] pkg_data = long_pkg != null ? long_pkg : recv_buf;
    while (recved > 0)
    {
      int pkg_size;
      int head_size;
      if (!tcp_packer.read_header(pkg_data, recved, out pkg_size, out head_size))
      {
        break;
      }
      if (recved < pkg_size)
      {
        break;
      }
      on_recv_client_cmd(pkg_data, head_size, pkg_size - head_size);
      if (recved > pkg_size)
      {
        recv_buf = new byte[RECV_LEN];
        Array.Copy(pkg_data, pkg_size, recv_buf, 0, recved - pkg_size);
        pkg_data = recv_buf;
      }
      recved -= pkg_size;
      if (recved == 0 && long_pkg != null)
      {
        long_pkg = null;
        long_pkg_size = 0;
      }
    }
  }
  void on_recv_client_cmd(byte[] data, int offset, int data_len)
  {
    cmd_msg msg;
    proto_man.unpack_cmd_msg(data, offset, data_len, out msg);
    if (msg != null)
    {
      lock (net_events)
      {
        net_events.Enqueue(msg);
      }
    }
  }
  void on_connect_timeout()
  {
    Debug.Log("connection time out");
  }
  void on_connect_error(String info)
  {
    Debug.Log(info);
  }
  void close()
  {
    if (!is_connect)
    {
      return;
    }
    if (recv_thread != null)
    {
      recv_thread.Abort();
    }
    if (client_socket != null && client_socket.Connected)
    {
      client_socket.Close();
    }
    is_connect = true;
  }
  private void on_send_data(IAsyncResult iar)
  {
    try
    {
      Socket client = (Socket)iar.AsyncState;
      client.EndSend(iar);
    }
    catch (System.Exception e)
    {
      Debug.Log(e.ToString());
    }
  }
  public void send_protobuf_cmd(int stype, int ctype, ProtoBuf.IExtensible body)
  {
    byte[] cmd_data = proto_man.pack_protobuf_cmd(stype, ctype, body);
    if (cmd_data == null)
    {
      return;
    }
    byte[] tcp_pkg = tcp_packer.pack(cmd_data);
    client_socket.BeginSend(
      tcp_pkg,
      0,
      tcp_pkg.Length,
      SocketFlags.None,
      new AsyncCallback(on_send_data),
      client_socket);
  }
  public void send_json_cmd(int stype, int ctype, string json_body)
  {
    byte[] cmd_data = proto_man.pack_json_cmd(stype, ctype, json_body);
    if (cmd_data == null)
    {
      return;
    }
    byte[] tcp_pkg = tcp_packer.pack(cmd_data);
    client_socket.BeginSend(
      tcp_pkg,
      0,
      tcp_pkg.Length,
      SocketFlags.None,
      new AsyncCallback(on_send_data),
      client_socket);
  }
  public void add_service_listener(int stype, net_message_handle handle)
  {
    if (event_listeners.ContainsKey(stype))
    {
      event_listeners[stype] += handle;
    }
    else
    {
      event_listeners.Add(stype, handle);
    }
  }
  public void remove_service_listener(int stype, net_message_handle handle)
  {
    if (!event_listeners.ContainsKey(stype))
    {
      return;
    }
    else
    {
      event_listeners[stype] -= handle;
      if (event_listeners[stype] == null)
      {
        event_listeners.Remove(stype);
      }
    }
  }
}
