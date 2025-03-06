#ifndef MATERIAL_projector_H
#define MATERIAL_projector_H
#include <glm/glm.hpp>  
#include <glm/ext.hpp>

#include "texture.h"
#include "box3.h"

struct projector {
	glm::mat4 view_matrix,proj_matrix;
    float distance_light;

	texture tex;

	glm::mat4 set_projection(glm::mat4 _view_matrix, float area_size) {
		view_matrix = _view_matrix;

		/* TBD: set the view volume properly so that they are a close fit of the 
		bounding box passed as parameter */
		proj_matrix =  glm::ortho(-area_size, area_size, -area_size, area_size,0.f, distance_light*2.f);
//		proj_matrix = glm::perspective(3.14f/2.f,1.0f,0.1f, distance_light*2.f);
		return proj_matrix;
	}
    
	glm::mat4 light_matrix() {
		return proj_matrix*view_matrix;
	}

	// size of the shadow map in texels
	int sm_size_x, sm_size_y;
};

#endif