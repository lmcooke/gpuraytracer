//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////																	 /////
/////        			 CS123 FINAL PROJECT README        				 /////
/////																	 /////
/////					    BY KENJI ENDO (CKENDO)						 /////
/////   				      LUCI COOKE (LMCOOKE)			 			 /////
/////  				 		  EMMA HEROLD (EMMA)			 			 /////
/////																	 /////
/////																	 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////							  THE GOAL								 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

We set out to make a gpu raytracer, with the idea of mimicking the real-time
interactivity of image editing and 3D-modeling software like Photoshop or
Maya.

We created a stochastic, real-time GPU raytracer with the idea of making it a
tool that a CG artist would use to view their changes to shading, lighting,
and camera position in real time. The raytracer converges on a more beautiul
render over time if the artist does not make any edits. Though the direction 
and scope of our project evolved over the early planning stages, this shook 
out to be this set of features:

	* A raytracer on the gpu
	* Runs real time
	* Implements the standard shapes (cone, cylinder, cube, sphere)
	* Implements the standard lighting equation from Ray (ambient, diffuse,
	  spec, shadows, reflections, texture mapping.
	* Has options for normal mapping, ambient occlusion, and depth of field
	* Can shoot rays stochastically as a form of anti-aliasing and enabling
	  samples to accumulate for ambient occlusion and depth of field.
	* When more computationally expensive featuers are enabled, the raytracer
	  blends the results of subsequent raytracing passes over time to create
	  progressively more beautiful images.

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////							  USAGE 								 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

Toggle between Scene 1, 2, 3. Use sliders to modify lighting intensities.

Stochastic sampling to reduce aliasing is most noticable with normal mapping/textures
on. 

Just AO is on if no other lighting components are on, if it is on in combination,
it will be composited. Stochastic sampling with just AO on demonstrates the
convergence well. Anytime you click or move the camera it will reset the FBOs.

Depth of field with a focal distance ~ 15 on scene 3 shows it off well. With
stochastic sampling on you will see the depth of field enhance

If stochastic sampling is on, a default of 1 ray is shot for DOF, 6 for AO.
if Stochastic sampling is OFF, you can change the number of samples shot for AO/DOF.

Stochastic sampling will not work with animation by convention (because it will
average with the previous frame, which will cause motion blur).

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						   DESIGN DECISIONS							 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

The bulk of our key pipeline and raytracing is in view.cpp and ray.frag,
respectively.

In view.cpp, we gather all the information about settings, the scenes, camera,
and lights and pass that information to the ray sahder as uniforms. In
view.cpp, we ping pong between two FBOS which alternate functioning as the 
previous or next FBO. This allows us to use the scene's previous color in it's
current color calculations, thus allowing us to accumulate information over time
and calculate more samples over time. The prevFBO is bound to the ray.frag shader,
and the shader will sample the previous 'render' and weight it in a contribution
with the current render, based on the number of samples (if we are on the 10th pass of
paintGL, the current render will be 1/10th of the final image). ray.frag writes to
nextFBO and also draws to the screen. FBOs are cleared out and the numPasses
reset to 0 any time there is a settings changed event (if the user moves the
orbit camera or modifies any of the UI sliders/toglges)

We have three preset scenes that are calculated in the static SceneBuilder class
on the CPU side. These scenes are built as lists of object structs, each of which 
contains information about the object's materials, textures, transformations, 
and primitive type. We were initially aiming to support arbitrary size scenes as a 
bell and whistle, using UBOs to pass structs lists into the shader. But for time, we 
decided instead to hardcode our scenes, built via SceneBuilder.cpp and sent to the 
GPU hardcoded as SceneObject, LightObject, and GlobalData structs. Our scenes support 
6 scene objects with 3 image textures/normals (or ulimited material types if just using params)
and have a global lighting setup with 3 lights (key, fill, point light), 2 environment map types.

The ray shader contains all of our ray tracing logic.

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						  RAY										 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

Migrating Ray/Intersect functionality over to the GPU ray.frag shader required 
some design changes. We still iterate through sceneObject and lightObject lists, 
but these are passed in individually as structs and assembled in main. 

To handle recursion for the reflected rays, we used a for loop with a max depth,
with a helper method, recursiveRayTrace.

Compositing/settings is done within ray.frag, based on the settingsData struct
passed in and the user's current settings on the UI. 

The meat of the lighting/compositing is done in calculateLighting.

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////		            STOCHASTIC SAMPLING 		   					 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

The FBO's ping pong in view.cpp's paintGL. In main in ray.frag, the primary ray
for this fragCoord 'pixel' is determined based on the aspect ratio. If stochastic 
sampling is on, it will jitter the UV position of this primary ray randomly within 
the 'pixel bound'. This stochastic samplign on it's on helps to antialias edges/textures,
and down the line, results in convergence for DOF and AO when the ray is left to sit
for a couple seconds.

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						  AMBIENT OCCLUSION 						 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

Ambient Occlusion was implemented in world space in the fragment shader. For
each fragment, a ray is cast into the scene to check for intersection. If there
is an intersection, a hemisphere is formed about the normal vector of intersection
and rays are randomly generated about the hemisphere. Each of these rays is checked
for intersection, and we integetrate over the distance at which each of these 
rays intersect an object. This implementation of Ambient Occlusion also allows
for the user to control the number of samples are being cast within the 
hemisphere. If stochastic sampling is off, the user can choose how many samples
will be used. If stochastic sampling is on, a default of 6 samples are used,
and the samples will be averaged out over time.


//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						    DEPTH OF FIELD 		   					 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

Depth of field is implemented in world space. The user can change the focal
length in the GUI, which internally shifts the film plane to be at
x = -focalLength and scales it to maintain the same size to z ratio as the
original film plane. Then, the eye point is jittered around the original eye
point by a maximum of the aperture size, which can also be modified in the GUI.
When stochastic sampling is enabled, DOF samples once per pass. When stochastic
sampling is disabled, DOF will sample based on the number of samples slider in
the GUI. This implementation of DOF is supposedly more physcially accurate than
other methods like depth map DOF, but it is more computationally expensive.


//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						    NORMAL MAPPING 		   					 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

For normal mapping, UV's are determined in object space per primitive via 
the same UV helpers as texture mapping (adjusted for the material's repeatU
and repeatV settings). The normals, in tangent space, are turned into object space
via TBN matrix. The normal, the z+ axis, is just the object space normal. The
object space bitangent is calculated per primitive (for example, for a cylinder's
drum, the bitangent will be y+). The tangent is calculated as the cross of the
normal and bitangent. 

From object space, the normal map normal is converted into world space to be used
in the rest of the calculateLighting equation.

//////////////////////////////////////////////////////////////////////////////
/////																	 /////
/////						 BUGS AND KNOWN ISSUES						 /////
/////																	 /////
//////////////////////////////////////////////////////////////////////////////

Resizing the CS123 Final window itself doesn't really resize the viewport correctly.
