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
  void Start()
  {
    add_status_option("Joined Room.");
    this.InvokeRepeating("test", 2.0f, 2.0f);
  }

  void test()
  {
    add_talk_option("127.0.0.1", 6080, "foobar");
    add_self_option("Hello");
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
    Debug.Log("on_enter_chat_room");
  }
  public void on_exit_chat_room()
  {
    Debug.Log("on_exit_chat_room");
  }
  public void on_send_msg()
  {
    Debug.Log("on_send_msg");
  }
}
