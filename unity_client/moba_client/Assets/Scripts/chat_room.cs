using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class chat_room : MonoBehaviour
{
  public ScrollRect scroll_view;
  public GameObject status_prefab;
  public GameObject chat_prefab;
  public GameObject self_prefab;
  public InputField input;
  private string sent_msg = null;

  void on_login_resp(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.LoginRes>(body);
    if (res.status == 1)
    {
      add_status_option("Join room successful");
    }
    else if (res.status == -1)
    {
      add_status_option("You are already in the room");
    }
  }
  void on_exit_resp(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.ExitRes>(body);
    if (res.status == 1)
    {
      add_status_option("Exit room successful");
    }
    else if (res.status == -1)
    {
      add_status_option("You are not in the room");
    }
  }
  void on_send_resp(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.SendMsgRes>(body);
    if (res.status == 1)
    {
      add_self_option(sent_msg);
    }
    else if (res.status == -1)
    {
      add_self_option("Send message unsuccessful");
    }
  }
  void on_user_login(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.OnUserLogin>(body);
    add_status_option(res.ip + ":" + res.port + " has joined the room");
  }

  void on_user_exit(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.OnUserExit>(body);
    add_status_option(res.ip + ":" + res.port + " has left the room");
  }

  void on_user_msg(byte[] body)
  {
    var res = proto_man.protobuf_deserialize<game.OnSendMsg>(body);
    add_talk_option(res.ip, res.port, res.content);
  }
  void on_chat_room_server_resp(cmd_msg msg)
  {
    switch (msg.ctype)
    {
      case (int)game.Cmd.eLoginRes:
        on_login_resp(msg.body);
        break;
      case (int)game.Cmd.eExitRes:
        on_exit_resp(msg.body);
        break;
      case (int)game.Cmd.eSendMsgRes:
        on_send_resp(msg.body);
        break;
      case (int)game.Cmd.eOnUserLogin:
        on_user_login(msg.body);
        break;
      case (int)game.Cmd.eOnUserExit:
        on_user_exit(msg.body);
        break;
      case (int)game.Cmd.eOnSendMsg:
        on_user_msg(msg.body);
        break;
    }
  }
  void Start()
  {
    network.instance.add_service_listener(2, on_chat_room_server_resp);
  }

  void add_status_option(string status_info)
  {
    var opt = GameObject.Instantiate(status_prefab);
    opt.transform.SetParent(scroll_view.content, false);
    Vector2 content_size = scroll_view.content.sizeDelta;
    content_size.y += 120;
    scroll_view.content.sizeDelta = content_size;
    opt.transform.Find("src").GetComponent<Text>().text = status_info;
    Vector3 local_pos = scroll_view.content.localPosition;
    local_pos.y += 120;
    scroll_view.content.localPosition = local_pos;
  }

  void add_talk_option(string ip, int port, string body)
  {
    var opt = GameObject.Instantiate(chat_prefab);
    opt.transform.SetParent(scroll_view.content, false);
    Vector2 content_size = scroll_view.content.sizeDelta;
    content_size.y += 120;
    scroll_view.content.sizeDelta = content_size;
    opt.transform.Find("src").GetComponent<Text>().text = body;
    opt.transform.Find("ip_port").GetComponent<Text>().text = ip + ":" + port;
    Vector3 local_pos = scroll_view.content.localPosition;
    local_pos.y += 120;
    scroll_view.content.localPosition = local_pos;
  }

  void add_self_option(string body)
  {
    var opt = GameObject.Instantiate(self_prefab);
    opt.transform.SetParent(scroll_view.content, false);
    Vector2 content_size = scroll_view.content.sizeDelta;
    content_size.y += 120;
    scroll_view.content.sizeDelta = content_size;
    opt.transform.Find("src").GetComponent<Text>().text = body;
    Vector3 local_pos = scroll_view.content.localPosition;
    local_pos.y += 120;
    scroll_view.content.localPosition = local_pos;
  }

  void Update()
  {

  }
  public void on_enter_chat_room()
  {
    network.instance.send_protobuf_cmd(2, (int)game.Cmd.eLoginReq, null);
  }
  public void on_exit_chat_room()
  {
    network.instance.send_protobuf_cmd(2, (int)game.Cmd.eExitReq, null);
  }
  public void on_send_msg()
  {
    if (input.text.Length <= 0)
    {
      return;
    }
    var req = new game.SendMsgReq();
    req.content = input.text;
    sent_msg = input.text;
    network.instance.send_protobuf_cmd(2, (int)game.Cmd.eSendMsgReq, req);
  }
}
