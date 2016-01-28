local times =  0

function loop()
    if(controller.elapsed(5000))
    then
        status = controller.status()
        controller.log("roll="..status.roll)
        controller.log("pitch="..status.pitch)
        controller.log("longitude="..status.longitude)
        controller.log("this is "..times)
        times = times + 1
    end
    return 0
end

print "Lua script start 2"
