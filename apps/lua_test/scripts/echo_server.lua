function echo_recv_cmd(s, msg)
  print(msg[1]) -- stype
  print(msg[2]) -- ctype
  print(msg[3]) -- utag
  local body = msg[4]
  print(body.name)
  print(body.age)
  print(body.email)
  print(body.int_set)
  local to_client = {
    1,
    2,
    0,
    {status = 200},
  }
  session.send_msg(s, to_client)
end

function echo_session_disconnect(s)
end

local echo_service = {
  on_session_recv_cmd = echo_recv_cmd,
  on_session_disconnect = echo_session_disconnect,
}

local echo_server = {
  stype = 1,
  service = echo_service,
}

return echo_server
