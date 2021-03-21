using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class game_scene : MonoBehaviour
{
  void Start()
  {
    this.Invoke("test", 4.0f);
    network.instance.add_service_listener(1, on_service_event);
  }

  void on_service_event(cmd_msg msg)
  {
    switch (msg.ctype)
    {
      case 2:
        var res = proto_man.protobuf_deserialize<game.LoginRes>(msg.body);
        Debug.Log(res.status + " status...");
        break;
    }
  }
  void OnDestroy()
  {
    if (network.instance)
    {
      network.instance.remove_service_listener(1, on_service_event);
    }
  }

  void test()
  {
    var req = new game.LoginReq();
    req.name = "fook";
    req.email = "q@q.com";
    req.age = 99;
    req.int_set = 8;
    network.instance.send_protobuf_cmd(1, 1, req);
  }
  void Update()
  {

  }
}
