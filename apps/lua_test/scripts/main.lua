
logger.init("logger/netbus", "netbus_log", 1);

local proto_type = {
  PROTO_JSON = 0,
  PROTO_BUF = 1,
}
proto_man.init(proto_type.PROTO_BUF)

if proto_man.proto_type() == proto_type.PROTO_BUF then
  local cmd_name_map = require("cmd_name_map")
  if cmd_name_map then
    proto_man.register_protobuf_cmd_map(cmd_name_map)
  end
end

netbus.tcp_server(6080)
netbus.tcp_server(6081)
netbus.ws_server(8001)
netbus.udp_server(8002)

print("Lua starting up!")
