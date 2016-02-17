local times =  0

function loop()
    if(controller.elapsed(2000))
    then
		times = times + 1
        if times > 0 and times <= 10 then
			controller.led(1,times * 10)
		elseif times > 10 and times <= 20 then
			controller.led(2,times * 10 - 100)
		elseif times == 21 then
			controller.thruster(1,50)
		elseif times == 22 then
			controller.thruster(2,50)
		elseif times == 23 then
			controller.thruster(3,50)
		elseif times == 24 then
			controller.thruster(4,50)
		elseif times == 25 then
			controller.thruster(5,50)
		elseif times == 26 then
			controller.thruster(6,50)
		else
			controller.led(1,0)
			controller.led(2,0)
			controller.thruster(1,0)
			controller.thruster(2,0)
			controller.thruster(3,0)
			controller.thruster(4,0)
			controller.thruster(5,0)
			controller.thruster(6,0)
			times = 0
		end
    end
    return 0
end

print "Lua script start 2"
