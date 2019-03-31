use <scad-utils/linalg.scad>
use <scad-utils/transformations.scad>

//////////////////////////////////////////////////////////////////////////////////
//
//  This library is strongly based on the Oskar Linde's library sweep.scad found in
//
//      https://github.com/openscad/list-comprehension-demos
//
//  Note that it requires some libraries from scad-utils which can be found in
//
//      https://github.com/OskarLinde/scad-utils
//
//  My work on it was:
//      1. incorporate the minimum twist code Oskar Linde posted in the Openscad Forum 
//         (http://forum.openscad.org/Twisty-problem-with-scad-utils-quot-sweep-quot-tc9775.html)
//      2. polishing some functions (simplifying some, allowing tail-recursion eliminination, etc)
//      3. include comments
//      4. add three helper functions to allow the control of sweep twists:
//              - adjusted_rotations(path_transf, angini=0, angtot=0, closed=false)
//              - adjusted_directions(path_transf, v0, vf=undef, turns=0, closed=false)
//              - referenced_path_transforms(path, vref, closed)
//      5. include the possibility of the user computation of the path tangents
//
//  The last helper function is a substitute to the original construct_transform_path(path, closed=false)
//  It is useful to constraint the sweep rotations to keep the sections aligned with a surface normal.
//  See the SweepDemo.scad for examples of usage.
//
//      Ronaldo Persiano
//
//////////////////////////////////////////////////////////////////////////////////////////
//
//  API functions
//  =============
//
//  Although some other functions may have their application to user codes, the main API function are:
//
//  module sweep(shape, path_transforms, closed=false)
//  --------------------------------------------------
//  It builds a polyhedron representing the sweep of of a 2D polygon shape along a path implicit in the
//  path_transforms sequence. This sequence is computed by other functions in the library or coded
//  by the user. This module arguments are:
//
//      shape           - a list of 2d vertices of a simple polygon
//      path_transforms - a sequence of affine transforms (4x4 matrices) to be applied to the shape
//      closed          - true if the path of the path_transforms is closed
//
//  function sweep_polyhedron(shape, path_transforms, closed=false, caps=true, inv=false)
//  ---------------------------------------------------------------
//  The function applies each transform path_transforms[i] to the shape and builds an 
//  envelope to the transformed shapes. If closed==true, connects the last to the 
//  first one. Otherwise, builds a cap at each end of the envelope acccording to caps. 
//  The resulting envelope data is returned as a polyhedron primitive input data [ points, faces ].
//  This function is called by module sweep(). It has, however, its own value in 
//  non-linear deformation of sweeping models. The function arguments are:
//
//      shape           - a list of 2d vertices of a simple polygon
//      path_transforms -   a sequence of affine transforms (4x4 matrices).
//      closed      - true if the path of the path_transforms is closed
//      caps        - either a boolean or a list of two booleans
//                    caps=[b1,b2] -> a cap is added at the begining (end) of the path 
//                                    if and only if b1 (b2) is true
//                    caps=[c]     -> equivalent to [c,c] 
//                    caps=[]      -> equivalent to [false,false]
//                    caps=true    -> equivalent to [true,true]; it is the default
//                    caps=false   -> equivalent to [false,false]
//                    caps is ignored if closed=true
//      inv         - the orientation of all faces are reverted if inv=true; deffault false
//
//  It is called by module sweep.
//
//  function construct_transform_path(path, closed=false, tangts)
//  ------------------------------------------------------------
//  Construct a list of rigid body transforms mapping the 3D origin to points of the path.
//  This is the fundamental base for the sweeping method which maps a section by the 
//  transforms and builds their envelope. If the argument tangts is given, that list
//  of the tangent at each point of path is taken instead of the internal computed sequence.
//  Returns a path transform list to feed sweep().
//
//  function adjusted_rotations(path_transf, angini=0, angtot=0, closed=false) 
//  --------------------------------------------------------------------------
//  Given an already built path transforms 'path_transf', this function adds (or subtract) 
//  an additional twist of value 'angtot' to each path_transf after rotating the initial section 
//  by 'angini'. If both angini and angtot are zero, the path_transf is unchanged.
//  Returns the resulting a new path transform list to feed sweep().
//
//  function adjusted_directions(path_transf, v0, vf=undef, turns=0, closed=false)
//  ------------------------------------------------------------------------------
//  Given an already built path transforms 'path_transf', this function adjust it in such a 
//  way that the x-axis of the section is mapped to the direction nearest to v0 
//  at the path start and to the direction nearest to vf at the path end. 
//  Additional twist turns (360 degrees twist) may be added or subtracted. 
//  Returns the resulting path transform list.
//
//  function referenced_path_transforms(path, vref, closed=false, tangts)
//  ---------------------------------------------------------------------
//  This function is an alternative to construct_transform_path. 
//  Given a path and a vector vref[i] for each point path[i],  the function computes 
//  a path transform list for sweeping such that the y-axis of the section is mapped 
//  to the vector vref[i] at point path[i]. The normal to the mapped section plane
//  is taken as the vector orthogonal to vref[i] nearest to the tangent of the path
//  at point path[i]. If the argument tangts is given, this list of the tangent at each
//  point of path is taken instead of tangent_path(path).
//  Returns the resulting path transform list.
//
//  module sweep_sections(shape, path_transforms)
//  ---------------------------------------------
//  This module has only debugging purposes. It draws just the 2d sections at the positions
//  of the path that are mapped by sweep.
//  
//  See SweepDemo.scad for usage.
//
// module lazzyUnion(pdats) 
// ------------------------
// This module allows the "union" of the polyhedron data in the list pdats. Each entry pdats[i] is supposed
// to be in the polyhedron primitive input data format [ points, faces ]; all points are concatenated in a
// one list and the faces as well after they are updated to reflect the new order of its vertices; 
// after that, the module make an unique call to polyhedron. No check is made for self-intersections and 
// non-manifoldness of the joining.
//
//////////////////////////////////////////////////////////////////////////////////////////

function unit(v) = norm(v)>0 ? v/norm(v) : undef; 

function transpose(m) = // m is any retangular matrix of objects
  [ for(j=[0:len(m[0])-1]) [ for(i=[0:len(m)-1]) m[i][j] ] ];

function identity(n) = [for(i=[0:n-1]) [for(j=[0:n-1]) i==j ? 1 : 0] ];

// computes the rotation with minimum angle that brings a to b
// ***** good just for unitary a and b ****
function rotate_from_to(a,b) = 
    let( axis = cross(a,b) )
    axis*axis>0.0001? let( ax = unit(axis) )
    transpose([b, ax, cross(ax, b)]) * [a, ax, cross(ax, a)] :
        identity(3); 

// generates the sequence of all partial rotations for each path point tangent
function rotations(tgts) = 
  [for( i  = 0, 
        ax = cross([0,0,1],tgts[0]),
        R  = tgts[0][2]>=0 || ax*ax >= 0.0001 ? 
                rotate_from_to([0,0,1],tgts[0]) : 
                [[1,0,0],[0,-1,0],[0,0,-1]];
        i < len(tgts);
        R = i<len(tgts)-1? rotate_from_to(tgts[i],tgts[i+1])*R : undef,
        i=i+1 )
    R ];

// computes the sequence of unitary path tangents to the given path. 
// If closed==true, assumes the path is closed.
function tangent_path(path, closed) =
    let( l = len(path) )
    closed ?
        [ for(i=[0:l-1]) unit(path[(i+1)%l]-path[(l+i-1)%l]) ] :
        let( t0 = l<4 ? 
                      unit(path[1]-path[0]) : 
                      unit(2*(path[2]-path[0]) - (path[3]-path[1])),
             tn = l<4 ?
                      unit(path[l-1]-path[l-2]):
                      unit((path[l-4]-path[l-2])-2*(path[l-3]-path[l-1])))
        concat( [ t0 ], 
                [ for(i=[1:l-2]) unit(path[i+1]-path[i-1]) ],
                [ tn ]
              );

// This function is not used anywhere here. 
// Computes an alternative sequence of path unitary tangents to the given path. 
// If closed==true, assumes the path is closed.
// Its output may be used as the argument tangts in the following functions
function tangents(spine, closed=false) = 
    let( n = len(spine) )
    closed?
        [ for(i=[0:n-1]) unit(spine[(n+i-2)%n] - 8*spine[(n+i-1)%n] + 8*spine[(i+1)%n] - spine[(i+2)%n]) ] :
        concat(
          [ unit(-25*spine[0] +48*spine[1] -36*spine[2] +16*spine[3] -3*spine[4]),
            unit(- 3*spine[0] -10*spine[1] +18*spine[2] - 6*spine[3] +  spine[4]) ]
             ,
          [ for(i=[2:n-3]) unit(spine[i-2] - 8*spine[i-1] + 8*spine[i+1] - spine[i+2]) ]
             ,
          [ unit( 3*spine[n-1] +10*spine[n-2] -18*spine[n-3] + 6*spine[n-4] -  spine[n-5]),
            unit(25*spine[n-1] -48*spine[n-2] +36*spine[n-3] -16*spine[n-4] +3*spine[n-5]) ]
         );

// builds the composition of rotation matrix r, 3x3, and translation by vector t in a 4x4 matrix
function construct_rt(r,t) = 
    [ concat(r[0], t[0]), concat(r[1],t[1]), concat(r[2], t[2]), [0,0,0,1] ];

// Given two rotations A and B, calculates the angle between B*[1,0,0] 
// and A*[1,0,0] that is, the total torsion angle difference between A and B.
function calculate_twist(A,B) = 
    let( D = transpose(B) * A)  
    atan2(D[1][0], D[0][0]); 

function construct_transform_path(path, closed=false, tangts) = 
   let( l = len(path),
        tangents = tangts==undef ? tangent_path(path, closed) : tangts,
        rotations = rotations(tangents),
        twist = closed ? calculate_twist(rotations[0], rotations[l-1]) : 0 )
   [ for (i = [0:l-1]) construct_rt(rotations[i], path[i]) * rotation( [0, 0, twist*i/(l-1)] ) ];

function adjusted_rotations(path_transf, angini=0, angtot=0, closed=false) = 
     let( l    = len(path_transf),
          atot = closed ? 360*floor(angtot/360)/(l-1) : angtot/(l-1) )
     [ for(i=[0:l-1]) path_transf[i]*rotation([0,0,atot*i+angini]) ];

function adjusted_directions(path_transf, v0, vf=undef, turns=0, closed=false) = 
     let( vp0  = [v0[0],v0[1],v0[2],0]*path_transf[0],
          ang0 = atan2(vp0[1], vp0[0]),
          vpf  = [vf[0],vf[1],vf[2],0]*path_transf[len(path_transf)-1],
          twst = vf == undef ? 0 : atan2(vpf[1], vpf[0]) - ang0,
          angf = turns*360 + twst )
     adjusted_rotations(path_transf, angini=ang0, angtot=angf, closed=closed);

function referenced_path_transforms(path, vref, closed=false, tangts) =
    let( l     = len(path),
         tgts  = tangts==undef ? tangent_path(path, closed) : tangts,
         vunit = [ for(v=vref) unit(v) ],
         // project tgts[i] in the plane orthogonal to vref[i]
         tgtr  = [ for(i=[0:l-1]) tgts[i]-(tgts[i]*vunit[i])*vunit[i] ],
         // builds the frame 
         rots  = [ for(i=[0:l-1]) 
                    let( vcross = unit(cross(tgtr[i], vunit[i])) )
                    vcross != undef ?
                        transpose([ vcross, vunit[i], tgtr[i] ]):
                        identity(3) 
                ] )
    [ for (i = [0:l-1]) construct_rt(rots[i], path[i]) ];

function sweep_polyhedron(shape, path_transforms, closed=false, caps=true, inv=false) = 
    let( ns       = len(shape),
         np       = len(path_transforms),
         segs     = np + (closed ? 0 : -1),
         shape3d  = to_3d(shape),
         range    = inv ? [np-1: -1: 0] : [0:np-1],
         verts    = [ for ( i   = range, 
                            pts = transform(path_transforms[i], shape3d) )
                         pts ],
         faces    = [ for( s  =[0:segs-1],  i = [0:ns-1], 
                           s0 = (s%np)*ns, s1 = ((s+1)%np)*ns )
                        [ s0+i, s1+i, s1+(i+1)%ns, s0+(i+1)%ns ] ],
         cap     =  closed || len(caps)==0 ? [false,false]:
                    (len(caps)==undef)?      [caps,caps] :
                    (len(caps)==1) ?         [caps[0], caps[0]] :
                    inv ?                    [caps[1], caps[0]] :
                                             [caps[0], caps[1]],
         bcap    = [ if(cap[0]) [ for (i=[0:ns-1]) i ] ],
         ecap    = [ if(cap[1]) [ for (i=[ns-1:-1:0]) i+ns*(np-1) ] ] )
    [ verts, concat(faces,bcap, ecap) ] ;

module sweep(shape, path_transforms, closed=false) {
    polyh = sweep_polyhedron(shape, path_transforms, closed) ;
    polyhedron(
        points = polyh[0], 
        faces  = polyh[1], 
        convexity = 5
    );
}

// for debug purpose, show just the sweep sections along the path
module sweep_sections(shape, path_transforms) {
    pathlen  = len(path_transforms);
    segments = pathlen + (closed ? 0 : -1);
    shape3d  = to_3d(shape);
    sweep_points = [ for ( i=[0:pathlen-1], pts = transform(path_transforms[i], shape3d) ) pts ];
    sections_facets = let (facets = len(shape3d))
                      [ for( i=[0:pathlen-1])
                            [ for( j=[0:facets-1] ) facets*i + j ] ];
    polyhedron(
        points = sweep_points,
        faces  = sections_facets,
        convexity = 5
    );
}

module lazyUnion(pdats) {
  verts = [for(pdat=pdats) each pdat[0] ];
  lens  = accum_sum([0, for(pdat=pdats) len(pdat[0]) ]);
  faces = [for(i=[0:len(pdats)-1])
             for(fac=pdats[i][1]) [for(f=fac) f+lens[i] ] ];
  polyhedron(verts, faces);  
} 



////////////////////////////////////////////
//
//  This demo illustrates the effect of four path transform functions
//  on the final sweep. The path transform considered are:
//      - for "non constraint":     construct_transform_path()
//      - for "angle constrained":  adjusted_rotations()
//      - for "on surface":         referenced_path_transforms()
//      - for "vector constrained"  adjusted_directions()
//
//  Three different examples of sweeps are included:
//      - sweep on a cylindrical helix
//      - sweep of a torus knot
//      - sweep on the surface of a function graph
//
//  For better interaction, it is recommended to run the code with the
//  customizer snapshot version 2016.08.18.
//
//      Written by Ronaldo Persiano
//
/////////////////////////////////////////////////

/////////// Customizer Parameters //////////////

/* [0. Description] */
// Ilustrates the extra functions in sweep2.scad to build path transforms for a path on a surface.
Example = "Torus knot"; // ["Helix sweep", "Torus knot", "Sweep on Surface"]

/* [1. Section parameters] */
section_shape          = "circular"; // [circular, star, half circle]
section_discretization = 12; // [3:36]
section_radius         = 30; // [5:40]

/* [2. Path parameters] */
// for all
path_discretization    = 20; // [10:100]
// for Torus knot
closed_path            = false; // [true, false]
// for Helix
helix_turns            = 3; // [0.4:0.1:3]
// Torus knot parameter (p,q)
knot = [2,3]; // 

/* [3.Sweep parameters] */
// constraint type on sweeping
sweep_constraint = "on surface"; // [no constraint, angle constrained, vector constrained, on surface]
// for angle constrained only
twist_turns   = 0; // [-3:3]
// for angle constrained only
initial_angle = 0; // [-180:180]
// for open paths and angle constrained only
final_angle   = 0;   // [-180:180]
// initial reference vector for vector constrained only
ref_ini = [0,0,100];
// final reference vector for vector constrained only
ref_end = [0,0,100];

/* [4. Surface parameters] */
// Undelying surface mesh size
surface_discretization = 50; // [10:80]

/* [5. Visualization parameters] */
// Show just the sections mapped by sweep
show_sections = false;
// Sweep frame: x red, y green, path blue, tangent not shown
show_frame = false;
// Sweep 
show_sweep = true;
// Undelying surface
show_surface = true; // 
/* [Hidden] */

/////////// Code starts here //////////////////
sd = section_discretization;
sr = section_radius;
pd = path_discretization;
if(Example=="Helix sweep") {
    // helix data
    R     = 300;
    pass  = 300;
    turns = helix_turns;
    if(show_surface) %cylinder(r=R, h=1.1*turns*pass, $fn=surface_discretization);
    path    = [for(i=[0:turns*pd]) let(a = i/pd)
                [R*cos(360*a), R*sin(360*a), a*pass] ];
    normals = [for(i=[0:turns*pd]) let(a = i/pd)
                [R*cos(360*a), R*sin(360*a), 0] ];
    do_sweep(path, normals, false);
}
if(Example=="Torus knot") {
    // Torus and knot data
    R = 400;
    r = 150;
    p = knot[0];
    q = knot[1];
    if(show_surface) %torus(R,r);
    k = max(p , q) * pd;   
    kc = closed_path ? 0 : 1;
    path    = [ for (i=[0:k-1-kc]) 
                knot(360*(i/(k-1))/gcd(p,q),R,r,p,q) ];
    normals = [ for (i=[0:k-1]) 
                knot_normal(360*i/(k-1)/gcd(p,q),R,r,p,q) ];
    do_sweep(path, normals, closed_path);
}
if(Example=="Sweep on Surface") {
    // the surface mesh
    nx = surface_discretization; 
    ny = surface_discretization;
    xmax = 500; ymax = 500;
    if(show_surface) {
        mesh = [for(i=[0:nx])
                [for(j=[0:ny])
                  let( x = -xmax+2*i*xmax/nx,
                       y = -ymax+2*j*ymax/ny )
                  [x, y, surface(x/xmax,y/ymax)] ] ];
        %draw_mesh(mesh,thickness=0);
    }
    // path on surface
    x0=[-xmax/2,-ymax];
    x1=[ xmax,   ymax];
    path=[for(i=[0:pd]) 
            let( x = x0[0]*(1-i/pd) + x1[0]*i/pd,
                 y = x0[1]*(1-i/pd) + x1[1]*i/pd )
            [ x, y, surface(x/xmax,y/ymax)] ];
    // normal at path points
    normals = [for(p=path) surf_normal(p[0]/xmax, p[1]/ymax, xmax,ymax) ];
    do_sweep(path, normals, false);
}

module do_sweep(path, normals, closed, tgts) {
    path_transf = construct_transform_path(path, closed, tgts);
    tot_ang     = 
        closed ? 
            360*twist_turns : 
            final_angle-initial_angle+360*twist_turns;
    adjusted_transf = 
        sweep_constraint == "angle constrained" ?
            adjusted_rotations( path_transf, 
                                angini = initial_angle, 
                                angtot = tot_ang, 
                                closed = closed):
        sweep_constraint == "vector constrained" ?
            adjusted_directions(path_transf, v0=ref_ini, vf=ref_end, turns=twist_turns, closed=closed) :
        sweep_constraint == "on surface" ?
            referenced_path_transforms(path, normals) :
            path_transf;
    
    if(show_frame) {
        projct = [[1,0,0,0],[0,1,0,0],[0,0,1,0]]; 
        framex = 100*[for(transf=adjusted_transf) projct*transf*[1,0,0,0]];
        framey = 100*[for(transf=adjusted_transf) projct*transf*[0,1,0,0]];
        for(i=[0:len(path)-1]) {
            color("red")   line(path[i],path[i]+framex[i], t= 10);
            color("green") line(path[i],path[i]+framey[i], t= 10);
        }
        color("blue") polyline(path, t=10);
    }
    if(show_sections && show_sweep) sweep_sections(section(), adjusted_transf, closed=closed);
    else if(show_sweep) sweep(section(), adjusted_transf, closed=closed);
}

function section() =
    section_shape=="circular" ?
        [for(i=[0:sd-1]) sr*[cos(i*360/sd), sin(i*360/sd)] ] :
    section_shape=="half circle" ?
        [for(i=[0:sd-1]) sr*[cos(i*180/(sd-1)),sin(i*180/(sd-1))] ]: 
    sr*[ [0,1], [-1/4,1/4], [-1,0], [-1/4,-1/4], 
         [0,-1], [1/4-1/4], [1,0],[1/4,1/4] ];

////////////////////////
// Torus knot functions

function knot(phi,R,r,p,q) = 
    [ (r * cos(q * phi) + R) * cos(p * phi),
      (r * cos(q * phi) + R) * sin(p * phi),
       r * sin(q * phi) ];

function knot_normal(phi,R,r,p,q) =  
    knot(phi,R,r,p,q) 
        - R*unit(knot(phi,R,r,p,q)
            - [0,0, knot(phi,R,r,p,q)[2]]) ;

function gcd(a, b) = (a == b)? a : (a > b)? gcd(a-b, b): gcd(a, b-a);

module torus(R,r){
    rotate_extrude() translate([R,0,0]) circle(r);
}
////////////////////////
// Surface functions

function surface(x,y) =
    10*(40 - 25*x*x - 25*y*y)*cos(180*x)*cos(180*y);

function dsurfdx(x,y) =
    -500*x*cos(180*x)*cos(180*y)
    - 10*(40 - 25*x*x - 25*y*y)*cos(180*y)*sin(180*x)*PI;

function dsurfdy(x,y) = dsurfdx(y,x);

function surf_normal(x,y,xmax,ymax) =
    [ -dsurfdx(x,y)/xmax, -dsurfdy(x,y)/ymax, 1]; // cross product

/////////////////////////
// Drawing modules for preview only
module draw_mesh(mesh) {
  n = len(mesh) != 0 ? len(mesh) : 0 ;
  m = n==0 ? 0 : len(mesh[0]) != 0 ? len(mesh[0]) : 0 ; 
  l = n*m;
  if( l > 0 ) {
    vertices = [ for(line=mesh) for(pt=line) pt ];     
    tris = concat(   [ for(i=[0:n-2],j=[0:m-2]) 
                        [ i*m+j, i*m+j+1, (i+1)*m+j ] ] ,
                     [ for(i=[0:n-2],j=[0:m-2]) 
                        [ i*m+j+1, (i+1)*m+j+1, (i+1)*m+j ] ] );
                   
    polyhedron(
        points = vertices,
        faces  = tris,
        convexity = 10
    ); 
  }
}
module line(p0, p1, t=1) {
    v = p1-p0;
    translate(p0)
        // rotate the cylinder so its z axis is brought to direction v
        multmatrix(rotate_from_to([0,0,1],v))
            cylinder(d=t, h=norm(v), $fn=4);
}

module polyline(p,t=1) {
    for(i=[1:1:len(p)-1]) line(p[i-1],p[i],t);
}
