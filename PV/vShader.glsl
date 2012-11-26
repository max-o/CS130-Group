#version 120
/*Phong shading*/
attribute vec4 vPosition;
attribute vec3 vNormal;

// output values will be interpreted per-fragment
varying vec3 fN;
varying vec3 fE;
varying vec3 fL;
varying vec4 transformedNormal;
varying vec4 transformedPosition;

uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform mat4 Projection;
uniform float shadingType;
//////////////////////////////////////////////
/*Gourad, flat shading*/
varying vec4 Normal;
varying vec4 color;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
///////////////////////////////////////////////

void main( void )
{
	if (shadingType == 0) // Phong Shading
	{

		transformedNormal      = ModelView * vec4(vNormal.x, vNormal.y, vNormal.z, 0);
		transformedPosition    = ModelView * vPosition;
	
	
		fN = vec3(transformedNormal.x,   transformedNormal.y,	transformedNormal.z);
		fE = -vec3(transformedPosition.x, transformedPosition.y, transformedPosition.z);	
	    
		fL = LightPosition.xyz; // light position is transformed to camera coordinates in main application
	

		if ( LightPosition.w != 0.0 )
		{
			fL = LightPosition.xyz - transformedPosition.xyz;
		}

		gl_Position = Projection * ModelView * vPosition;
	}

	else // Gourad, flat shading
	{
		Normal = vec4(vNormal.x, vNormal.y, vNormal.z, 0);
		vec4 ambient, diffuse, specular;
		gl_Position = Projection * ModelView * vPosition;	
		vec4 NN = ModelView*Normal;
		vec3 N = normalize(NN.xyz);
		vec3 L = normalize(LightPosition.xyz - (ModelView*vPosition).xyz);
		vec3 E = -normalize((ModelView*vPosition).xyz);
		vec3 H = normalize(L+E);
		float Kd = max(dot(L, N), 0.0);
		float Ks = pow(max(dot(N, H), 0.0), Shininess);
		ambient = AmbientProduct;
		diffuse = Kd*DiffuseProduct;
		specular = max(pow(max(dot(N, H), 0.0), Shininess) * SpecularProduct, 0.0);
		color = vec4((ambient + diffuse + specular).xyz, 1.0);
	}


}