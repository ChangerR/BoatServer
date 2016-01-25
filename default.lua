local times =  0

function loop()
    status = controller.status()
    print("roll="..status.roll)
    print("pitch="..status.pitch)
    print("longitude="..status.longitude)
    print("this is "..times)
    times = times + 1
    return 0
end

print "Lua script start"
