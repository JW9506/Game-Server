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
  private Socket client_socket = null;
  private bool is_connect = false;
  private Thread recv_thread = null;
  private byte[] recv_buffer = new byte[8192];
  void Awake()
  {
    DontDestroyOnLoad(this.gameObject);
  }
  void Start()
  {
    connect_to_server();
    // unity
    this.Invoke("test", 4.0f);
  }
  void test()
  {
    var req = new game.LoginReq();
    req.name = "fook";
    req.email = "q@q.com";
    req.age = 99;
    req.int_set = 8;
    this.send_protobuf_cmd(1, 1, req);
  }
  void Update() { }
  void connect_to_server()
  {
    try
    {
      client_socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
      var ipAddress = IPAddress.Parse(this.server_ip);
      var ipEndPoint = new IPEndPoint(ipAddress, this.port);
      var result = client_socket.BeginConnect(ipEndPoint, new AsyncCallback(on_connect_success), client_socket);
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
      recv_thread = new Thread(new ThreadStart(on_recv_data));
      recv_thread.Start();
      Debug.Log("Connected to " + server_ip + ":" + port);
    }
    catch (System.Exception e)
    {
      on_connect_error(e.ToString());
      is_connect = false;
    }
  }
  void on_recv_data()
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
        int revc_len = client_socket.Receive(this.recv_buffer);
        if (revc_len > 0)
        {
          Debug.Log("recv_len = " + revc_len);
          Debug.Log(this.recv_buffer.ToString());
        }
      }
      catch (System.Exception e)
      {
        Debug.Log(e.ToString());
        client_socket.Disconnect(true);
        client_socket.Shutdown(SocketShutdown.Both);
        client_socket.Close();
        is_connect = false;
        break;
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
  }
  void send_protobuf_cmd(int stype, int ctype, ProtoBuf.IExtensible body)
  {
    client_socket.Send(tcp_packer.pack(proto_man.pack_protobuf_cmd(stype, ctype, body)));
  }
  void send_json_cmd(int stype, int ctype, string json_body)
  {

  }
}
