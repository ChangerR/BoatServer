
function loop()
    if(controller.elapsed(5000))
    then
		status = controller.status()
		controller.log("laser1="..status.laser1.." laser2="..status.laser2.." laser3="..status.laser3.." laser4="..status.laser4.." laser5="..status.laser5)
    end
    return 0
end