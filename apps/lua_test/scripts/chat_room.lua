function on_chat_room_recv_cmd(s, msg)
end

function on_chat_room_session_disconnect(s)
  local ip, port = session.get_address(s)
  print("From chat room: >>> "..ip..":"..port.." disconnected")
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
