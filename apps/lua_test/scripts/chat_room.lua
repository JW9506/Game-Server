local session_set = {}

function broadcast_except(s, msg)
  for i = 1, #session_set do
    if s ~= session_set[i] then
      session.send_msg(session_set[i], msg)
    end
  end
end

function on_recv_login_cmd(s)
  for i = 1, #session_set do
    if s == session_set[i] then
      local msg = {2, 2, 0, {status = -1}}
      session.send_msg(s, msg)
      return
    end
  end
  table.insert(session_set, s)
  local msg = {2, 2, 0, {status = 1}}
  session.send_msg(s, msg)

  local ip, port = session.get_address(s)
  msg = {2, 7, 0, {ip = ip, port = port}}
  broadcast_except(s, msg)
end

function on_recv_exit_cmd(s)
  for i = 1, #session_set do
    if s == session_set[i] then
      table.remove(session_set, i)
      local msg = {2, 4, 0, {status = 1}}
      session.send_msg(s, msg)

      local ip, port = session.get_address(s)
      msg = {2, 8, 0, {ip = ip, port = port}}
      broadcast_except(s, msg)
      return
    end
  end
  local msg = {2, 4, 0, {status = -1}}
  session.send_msg(s, msg)
end

function on_recv_send_msg_cmd(s, content)
  for i = 1, #session_set do
    if s == session_set[i] then
      local msg = {2, 6, 0, {status = 1}}
      session.send_msg(s, msg)
      local ip, port = session.get_address(s)
      msg = {2, 9, 0, {ip = ip, port = port, content = content}}
      broadcast_except(s, msg)
      return
    end
  end
  local msg = {2, 6, 0, {status = -1}}
  session.send_msg(s, msg)
end

function on_chat_room_recv_cmd(s, msg)
  local ctype = msg[2];
  local body = msg[4]
  if ctype == 1 then
    on_recv_login_cmd(s)
  elseif ctype == 3 then
    on_recv_exit_cmd(s)
  elseif ctype == 5 then
    on_recv_send_msg_cmd(s, body.content)
  end
end

function on_chat_room_session_disconnect(s)
  local ip, port = session.get_address(s)
  for i = 1, #session_set do
    if s == session_set[i] then
      print("From chat room: >>> "..ip..":"..port.." disconnected")
      table.remove(session_set, i)
      return
    end
  end
end

local chat_service = {
  on_session_recv_cmd = on_chat_room_recv_cmd,
  on_session_disconnect = on_chat_room_session_disconnect,
}

local chat_room = {
  stype = 2,
  service = chat_service,
}

return chat_room
