local times =  0

function loop()
    if(controller.elapsed(5000))
    then
        status = controller.status()
        print("roll="..status.roll)
        print("pitch="..status.pitch)
        print("longitude="..status.longitude)
        print("this is "..times)
        times = times + 1
    end
    return 0
end

print "Lua script start"
