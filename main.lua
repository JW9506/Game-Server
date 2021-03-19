-- print("hell

--[[ function print_r ( t )
    local print_r_cache={}
    local function sub_print_r(t,indent)
        if (print_r_cache[tostring(t)]) then
            print(indent.."*"..tostring(t))
        else
            print_r_cache[tostring(t)]=true
            if (type(t)=="table") then
                for pos,val in pairs(t) do
                    if (type(val)=="table") then
                        print(indent.."["..pos.."] => "..tostring(t).." {")
                        sub_print_r(val,indent..string.rep(" ",string.len(pos)+8))
                        print(indent..string.rep(" ",string.len(pos)+6).."}")
                    elseif (type(val)=="string") then
                        print(indent.."["..pos..'] => "'..val..'"')
                    else
                        print(indent.."["..pos.."] => "..tostring(val))
                    end
                end
            else
                print(indent..tostring(t))
            end
        end
    end
    if (type(t)=="table") then
        print(tostring(t).." {")
        sub_print_r(t,"  ")
        print("}")
    else
        sub_print_r(t,"  ")
    end
    print()
end

log_debug("World1")
-- log_warning("Hello2")

mysql_wrapper.connect("127.0.0.1", 3306, "class_sql", "root", "root", function(err, context)
  log_debug("Lua sql connect call")
  if (err) then
    print(">> ", err)
    return
  end
  -- mysql_wrapper.close(context)
  mysql_wrapper.query(context, "select * from foo", function(err, result)
    if (err) then
      print(err)
      return
    end
    print_r(result)
  end)
end)

redis_wrapper.connect("127.0.0.1", 6379, function(err, context)
  log_debug("redis lua in!")
  redis_wrapper.query(context, "hmset 999 name 'foo' age '9'", function(err, result)
    if (err) then
      print(err)
      return
    end
    print(result)
  end)
  redis_wrapper.query(context, "hgetall 999", function(err, result)
    if (err) then
      print(err)
      return
    end
    print_r(result)
  end)
end)
-- local b = 3
-- b[1] = 0
 ]]


--[[ local my_service = {
  on_session_recv_cmd = function(session, msg)
  end,
  on_session_disconnect = function(session)
  end,
}

local ret = service.register(100, my_service)
print(ret) ]]

local timer = scheduler.schedule(function()
  print("hello world!!")
end, 2000, 1, 2000)

scheduler.once(function()
  scheduler.cancel(timer)
  print("timer canceled")
end, 3500)

logger.error("Foo")
