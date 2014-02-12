// Read datalog-all files and extract and plot features
clear;
clc;
clf();

fd = mopen('/home/rasmus/performand.k70.2/release/linux-cortexm-1.11.0/projects/performand/data/datalog-all105/datalog-all105.0.txt', 'r');
linecounter = {1,1,1,1,1,1,1,1,1,1};

magXaxis = zeros(2, 5000);
magYaxis = zeros(2, 5000);
magZaxis = zeros(2, 5000);

accXaxis = zeros(2, 5000);
accYaxis = zeros(2, 5000);
accZaxis = zeros(2, 5000);

imuPitch = zeros(2, 5000);
imuYaw = zeros(2, 5000);
imuRoll = zeros(2, 5000);

gpsCoor = zeros(2, 5000);

printf("Start Evaluation... ");
while(meof(fd) ~= null)
    line = mgetl(fd, 1);
    if isempty(line) then
       break;
    end
    
    tok = strsplit(line, ",")

    if strcmp(tok(2), "$COMPASS") == 0 then
        tmpsize = size(evstr(tok(3)));
        if tmpsize(2) > 0 then
            magXaxis(2, linecounter(1)) = evstr(tok(3));
            magXaxis(1, linecounter(1)) = evstr(tok(1));
            linecounter(1) = linecounter(1) + 1;
        end
        
        tmpsize = size(evstr(tok(4)));
        if tmpsize(2) > 0 then
            magYaxis(2, linecounter(2)) = evstr(tok(4));
            magYaxis(1, linecounter(2)) = evstr(tok(1));
            linecounter(2) = linecounter(2) + 1;
        end
        
        tmpsize = size(evstr(tok(5)));
        if tmpsize(2) > 0 then
            magZaxis(2, linecounter(3)) = evstr(tok(5));
            magZaxis(1, linecounter(3)) = evstr(tok(1));
            linecounter(3) = linecounter(3) + 1;
        end
                
        tmpsize = size(evstr(tok(6)));
        if tmpsize(2) > 0 then
            accXaxis(2, linecounter(4)) = evstr(tok(6));
            accXaxis(1, linecounter(4)) = evstr(tok(1));
            linecounter(4) = linecounter(4) + 1;
        end
        
        tmpsize = size(evstr(tok(7)));
        if tmpsize(2) > 0 then
            accYaxis(2, linecounter(5)) = evstr(tok(7));
            accYaxis(1, linecounter(5)) = evstr(tok(1));
            linecounter(5) = linecounter(5) + 1;
        end
        
        tmpsize = size(evstr(tok(8)));
        if tmpsize(2) > 0 then
            accZaxis(2, linecounter(6)) = evstr(tok(8));
            accZaxis(1, linecounter(6)) = evstr(tok(1));
            linecounter(6) = linecounter(6) + 1;
        end
    elseif strcmp(tok(2), "$IMU") == 0 then
        tmpsize = size(evstr(tok(5)));
        if tmpsize(2) > 0 then
            imuYaw(2, linecounter(7)) = evstr(tok(4));
            imuYaw(1, linecounter(7)) = evstr(tok(1));
            linecounter(7) = linecounter(7) + 1;
        end
        
        tmpsize = size(evstr(tok(5)));
        if tmpsize(2) > 0 then
            imuPitch(2, linecounter(8)) = evstr(tok(5));
            imuPitch(1, linecounter(8)) = evstr(tok(1));
            linecounter(8) = linecounter(8) + 1;
        end
        
        tmpsize = size(evstr(tok(6)));
        if tmpsize(2) > 0 then
            imuRoll(2, linecounter(9)) = evstr(tok(6));
            imuRoll(1, linecounter(9)) = evstr(tok(1));
            linecounter(9) = linecounter(9) + 1;
        end

    elseif strcmp(tok(2), "$GPS") == 0 then
        tmpsize = size(evstr(tok(11)));
        if tmpsize(2) > 0 then
            gpsCoor(2, linecounter(10)) = evstr(tok(11)) + evstr(tok(12))/60 + evstr(tok(13))/3600; // N -> latitude (Y)
            gpsCoor(1, linecounter(10)) = evstr(tok(15)) + evstr(tok(16))/60 + evstr(tok(17))/3600; // E -> longitude (X)
            linecounter(10) = linecounter(10) + 1;
        end
    end
end
mclose(fd);
printf("Stop Evaluation\n");

a = figure(1);
a.background=8;
subplot(3,1,1)
title('Magnetometer X-Y-Z')
plot(magXaxis(1,1:linecounter(1)-1), magXaxis(2,1:linecounter(1)-1), '-b');
plot(magYaxis(1,1:linecounter(2)-1), magYaxis(2,1:linecounter(2)-1), '-r');
plot(magZaxis(1,1:linecounter(3)-1), magZaxis(2,1:linecounter(3)-1), '-g');
al = legend('X-axis', 'Y-axis', 'Z-axis', 3);
al.background=8;
mtlb_axis([100, 1200, -1000, 1000])

subplot(3,1,2)
title('Accelerometer X-Y-Z')
plot(accXaxis(1,1:linecounter(4)-1), accXaxis(2,1:linecounter(4)-1), '-b');
plot(accYaxis(1,1:linecounter(5)-1), accYaxis(2,1:linecounter(5)-1), '-r');
plot(accZaxis(1,1:linecounter(6)-1), accZaxis(2,1:linecounter(6)-1), '-g');
al = legend('X-axis', 'Y-axis', 'Z-axis', 3);
al.background=8;
mtlb_axis([100, 1200, -40000, 40000])

subplot(3,1,3)
title('IMU Pitch-Yaw-Roll')
plot(imuYaw(1,1:linecounter(8)-1), imuYaw(2,1:linecounter(8)-1), '-r');
plot(imuPitch(1,1:linecounter(7)-1), imuPitch(2,1:linecounter(7)-1), '-b');
plot(imuRoll(1,1:linecounter(9)-1), imuRoll(2,1:linecounter(9)-1), '-g');
al = legend('Yaw', 'Pitch', 'Roll', 3);
al.background=8;
mtlb_axis([100, 1200, -200, 200])

b = figure(2);
b.background=8;
title('GPS coordinates')
plot(gpsCoor(1,1:linecounter(10)-1), gpsCoor(2,1:linecounter(10)-1), '.b');
//plot(gpsCoor(1,linecounter(10)-3), gpsCoor(2,linecounter(10)-3), '.r');

xs2pdf(a, 'imu_compass', 'landscape');
xs2pdf(b, 'found_gps', 'landscape');



