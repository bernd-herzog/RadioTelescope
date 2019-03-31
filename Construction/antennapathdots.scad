use <sweep.scad>

echo(version=version());

dx = 424.26;
dy = dx;
br = 10;
bl = 30;
nx = 2;
ny = 2;
Nx = 4;
Ny = 4;
Ax = 50;
Ay = 50;
g = 8;
L = 490;

function fx(z) =
  br * pow( 1- pow(1-z/bl,nx+Nx) ,1/nx);

function fx2(z) = br +
  (dx - br) * (pow(pow((z-bl)/Ax, nx+Nx) +1, 1/nx) -1)
            / (pow(pow((L-bl)/Ax, nx+Nx) +1, 1/nx) -1);


function fy(z) = 0.5 * g + 
  (dy - 0.5 * g) * (pow(pow(z/Ay,ny+Ny) +1,1/ny) -1)
                 / (pow(pow(L/Ay,ny+Ny) +1,1/ny) -1);

for (x=[0:200])
  color("green")
    translate([fx(x/100)/10, fy(x/100)/10, x/100/10])
      sphere(r = 0.01);


for (x=[2:bl])
  color("green")
    translate([fx(x)/10, fy(x)/10, x/10])
      sphere(r = 0.01);


for (x=[bl/2:bl*20/2])
  color("red")
    translate([fx2(x*2)/10, fy(x*2)/10, x*2/10])
      sphere(r = 0.01);

