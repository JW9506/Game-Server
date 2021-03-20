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
  void Awake()
  {
    DontDestroyOnLoad(this.gameObject);
  }
  void Start()
  {
    connect_to_server();
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
      Debug.Log("connect success");
    }
    catch (System.Exception e)
    {
      on_connect_error(e.ToString());
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
}
