-- print("hello")

log_debug("World1")
-- log_warning("Hello2")

mysql_wrapper.connect("127.0.0.1", 3306, "class_sql", "root", "root", function(err, context)
  log_debug("Lua sql connect call")
  if (err) then
    print(">> ", err)
    return
  end
  mysql_wrapper.close(context)
end)

-- local b = 3
-- b[1] = 0
