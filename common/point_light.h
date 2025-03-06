#ifndef MATERIAL_pointL_H
#define MATERIAL_pointL_H
#include <glm/glm.hpp>  
#include <glm/ext.hpp>

#include <vector>

#include "texture.h"


struct point_light
{
    glm::vec3 lightPos;
    std::vector<glm::mat4> light_matrices;
    texture cubemapDepth;

    void set(glm::vec3 position)
    {
        lightPos = position;
        float aspect = (float)sm_size_x/(float)sm_size_y;
        float near = 1.0f;
        float far = 25.0f;
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, near, far);

        light_matrices.push_back(proj * 
                 glm::lookAt(position, position + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        light_matrices.push_back(proj * 
                        glm::lookAt(position, position + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        light_matrices.push_back(proj * 
                        glm::lookAt(position, position + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        light_matrices.push_back(proj * 
                        glm::lookAt(position, position + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
        light_matrices.push_back(proj * 
                        glm::lookAt(position, position + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        light_matrices.push_back(proj * 
                        glm::lookAt(position, position + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));

        //cubemapDepth.create_cubemap(sm_size_x, sm_size_y, 1);
    }

    glm::mat4 light_matrix(int face)
    {
        return light_matrices[face];
    }

	// size of one face in texels
	int sm_size_x, sm_size_y;
};

// ogni istanza di head_light corrisponde ad un faro
struct headl_light
{
    glm::mat4 light_matrix;
    int sm_size_x, sm_size_y;

    void set(glm::mat4 view)
    {

        float aspect = (float)sm_size_x/(float)sm_size_y;
        float near = 0.1f;
        float far = 100.0f;
        //float distance_light = 0.5;
        //glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 0.1f * 2.0f);
        
        glm::mat4 proj = glm::perspective(glm::radians(90.0f), aspect, near, far);
        //glm::mat4 proj_matrix = glm::perspective(3.14f/2.f,1.0f,0.1f, distance_light*2.f);
        light_matrix = proj * view;
    
    }
    
};


#endif
