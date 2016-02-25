#thruster1: zuo main, 2: you main 3: you qian 4: zuo qian 5: you hou 6: zuo hou

d = 940 --distance between two laser on one side
a = 0
b = 0

function loop()

		status = controller.status()

		Lgap = status.laser1 - status.laser3
		Rgap = status.laser4 - status.laser2

		if (Lgap < 0) then
			Lgap1 = -Lgap
		else
			Lgap1 = Lgap
		end

		if (Rgap < 0) then
			Rgap1 = -Rgap
		else
			Rgap1 = Rgap
		end

		Angle_gap = (Lgap1 + Rgap1) / 2
		Angle = math.atan2(Angle_gap, d) * 180 / 3.14

				if(controller.elapsed(2000)) then
			controller.log("laser1="..status.laser1.." laser2="..status.laser2.." laser3="..status.laser3.." laser4="..status.laser4.." laser5="..status.laser5)
			controller.log("Rgap1="..Rgap1.." Lgap1="..Lgap1.." Angle="..Angle.." Rangle="..Rangle.." Fdis_Lvalue="..Fdis_Lvalue.." Fdis_Rvalue="..Fdis_Rvalue.."a"..a.."b="..b)
		end

		Rangle = 0
		dis_Lvalue = (status.laser1 + status.laser3) / 2
		dis_Rvalue = (status.laser4 + status.laser2) / 2
		Fdis_Lvalue = dis_Lvalue * math.cos(Angle) + 255
		Fdis_Rvalue = dis_Rvalue * math.cos(Angle) + 255

		if (Lgap > 0 or Rgap > 0) then  --clockwise
			Angle = Angle

		else if (Lgap < 0 or Rgap < 0) then--counter clockwise
			Angle = -Angle
		end

        if math.abs(Fdis_Lvalue - Fdis_Rvalue) <= 0 then
			a = 50 * math.sin(Angle)
			controller.thruster(1,0)
			controller.thruster(2,0)
			controller.thruster(6,-a)
			controller.thruster(5,a)
			controller.thruster(4,a)
			controller.thruster(3,-a)

		elseif Fdis_Lvalue - Fdis_Rvalue > 0 then
			a = 50 * math.sin(Angle)
			b = 100 * (Fdis_Lvalue - Fdis_Rvalue) / (Fdis_Lvalue + Fdis_Rvalue)
			controller.thruster(1,0)
			controller.thruster(2,0)
			controller.thruster(6,b-a)
			controller.thruster(5,-b + a)
			controller.thruster(4,b + a)
			controller.thruster(3,-b - a)

		elseif Fdis_Rvalue - Fdis_Lvalue > 0 then
			a = 50 * math.sin(Angle)
			b = 100 * (Fdis_Rvalue - Fdis_Lvalue) / (Fdis_Lvalue + Fdis_Rvalue)
			controller.thruster(1,0)
			controller.thruster(2,0)
			controller.thruster(3,b - a)
			controller.thruster(4,-b + a)
			controller.thruster(5,b + a)
			controller.thruster(6,-b - a)
		else
			controller.thruster(1,0)
			controller.thruster(2,0)
			controller.thruster(3,0)
			controller.thruster(4,0)
			controller.thruster(5,0)
			controller.thruster(6,0)
		end
    end
end
