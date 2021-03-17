#pragma once

struct cmd_msg;
class session;
class service {
  public:
    virtual bool on_session_recv_cmd(session* s, struct cmd_msg* msg);
    virtual void on_session_disconnect(session* s);
};
