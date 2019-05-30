# time unit is 128 microseconds
t_127=22 # very fast key pressure
velocity_min=1

function velocity = velocity(t_127, t, velocity_min)
  velocity = max (127.0 * t_127 ./ t, velocity_min); 
endfunction 

function velocity = velocity_2(t_127, t, velocity_min)
  velocity = max (127.0 * t_127^2 ./ t.^2, velocity_min); 
endfunction 


graphics_toolkit('gnuplot');

t_pp=300 # pianissimo
t=t_127:1:t_pp;
v = velocity(t_127,t,velocity_min);

t_pp_2=300 # pianissimo
t_2=t_127:1:t_pp;
v_2 = velocity_2(t_127,t,velocity_min);

figure(1)
plot(t,v,t_2,v_2);
xlabel("t / 128 us");
ylabel("velocity");
legend("1. order", "2. order");
title("MIDI velocity depending on time");

