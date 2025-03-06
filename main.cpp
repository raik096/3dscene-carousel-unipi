#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "3dparty/nanosvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "3dparty/nanosvg/src/nanosvgrast.h"

// Include OpenGL-related libraries
#include <GL/glew.h>      // Always first for OpenGL
#include <GLFW/glfw3.h>

// Standard library includes
#include <string>
#include <iostream>

#include <algorithm>
#include <sys/stat.h>
#include "common/gltf_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3dparty/tinygltf/stb_image.h"
#include "3dparty/tinygltf/stb_image_write.h"
#include "common/texture.h"

// Project-specific headers
#include "common/utilities.h"
#include "common/debugging.h"
#include "common/renderable.h"
#include "common/shaders.h"
#include "common/simple_shapes.h"
#include "common/carousel/carousel.h"
#include "common/carousel/carousel_to_renderable.h"
#include "common/carousel/carousel_loader.h"
#include "common/matrix_stack.h"
#include "common/intersection.h"
#include "common/trackball.h"
#include "common/trackball.h"
#include "common/projector.h"
#include "common/frame_buffer_object.h"
#include "common/point_light.h"

#define SHADOW_MAPPING true
#define CAR_HEADLIGHTS true
#define CAR_AMOUNT 2 //Il max dovrebbe essere dieci

/* creo un array di due oggetti di tipo trackball e curr_tb mi tiene traccia dell'indice attivo tralle due tb*/
trackball tb[2];
int curr_tb;

/* projection matrix*/
glm::mat4 proj;

/* view matrix */
glm::mat4 view;

matrix_stack stack;
float scaling_factor = 1.0;

//Shadow Mapping del sole
float depth_bias = 0;
float k_plane_approx = 0.5;

frame_buffer_object ligthDepthFbo;
projector Lproj;
glm::vec4 Ldir;

//Gestione delle viste dei cameramans
//Queste variabili sono importanti perchè se stiamo guardando da un camMan dobbiamo aggiornare la viewMatrix adeguatamente a ogni loop
int cameramanFocus = 0; //Id del cameraman dal quale vogliamo vedere.
bool watchCameras = false; //Se true vogliamo guardare attreverso gli occhi del cameraman, se false vogliamo usare la trackball per manipolare la camera "principale"

//Renderable Objects
box3 bbox_lamp, bbox_car, bbox_tree, bbox_drone, bbox_cube;
std::vector<renderable> lamp;
std::vector<renderable> car;
std::vector<renderable> tree;
std::vector<renderable> drone;
std::vector<renderable> cube;

renderable r_track;
texture track_texture;
renderable r_terrain;
texture terrain_texture;
texture white;

//Disegna la scena per il creare lo shadowMapping, l'fbo target deve essere bindato prima
void DrawDepthScene(race r, shader shader)
{
	glUseProgram(shader.program);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	stack.push();

	r_terrain.bind();
	shader.SetMatrix4x4("uModel", stack.m());
	BindTexture(shader, "uTexture", terrain_texture, 1);
	glDrawElements(GL_TRIANGLES, 390150, GL_UNSIGNED_INT, 0);

	//Track
	glDepthRange(0.01, 1);
	r_track.bind();
	shader.SetMatrix4x4("uModel", stack.m());
	BindTexture(shader, "uTexture", track_texture, 1);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, r_track.vn);
	glDepthRange(0.0, 1);

	//Drones
	for (unsigned int ic = 0; ic < r.cameramen().size(); ++ic) {
		stack.push();
		stack.mult(r.cameramen()[ic].frame);
		stack.mult(glm::translate(glm::scale(glm::mat4(1.f), glm::vec3(0.5f, 0.5f, 0.5f)), glm::vec3(0.0f, 0.f, 5.0f)));
		DrawModel(drone, shader, stack.m());
		stack.pop();
	}

	//Trees
	for (stick_object l : r.trees())
	{
		stack.push();
		stack.mult(glm::scale(glm::translate(glm::mat4(1), l.pos), glm::vec3(0.1)));
		DrawModel(tree, shader, stack.m());
		stack.pop();
	}

	//Lampioni
	lamp[0].bind();
	for (int i = 0;  i < r.lamps().size(); i++)
	{
		stick_object l = r.lamps()[i];
		stack.push();
		stack.mult(glm::scale(glm::translate(glm::mat4(1), l.pos), glm::vec3(0.2)));

		// Renderizza l'oggetto
		glUniformMatrix4fv(shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
		glDrawElements(lamp[0]().mode, lamp[0]().count, lamp[0]().itype, 0);
		stack.pop(); // Ripristina lo stato precedente
	}

	//Cars
	for (unsigned int k = 0; k < CAR_AMOUNT; ++k) {
		stack.push();
		
		stack.mult(glm::scale( r.cars()[k].frame, glm::vec3(0.5f, 0.5f, 0.5f)));
		DrawModel(car, shader, stack.m());
		stack.pop();
	}
	
	stack.pop();
	glUseProgram(0);
}

//Callback per fare gestire il resize della finestra
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
	proj = glm::perspective(glm::radians(45.f), (float)width/height, 0.1f, 100.f);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	tb[curr_tb].mouse_move(proj, view, xpos, ypos);
}

/* callback function called when a mouse button is pressed */
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

	/* Qui ho implementato il premere la rotella */
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		tb[curr_tb].mouse_middle_press(proj, view, xpos, ypos);
	}
	else
		if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
			tb[curr_tb].mouse_middle_release();
		}

	/* Qui ho implementato il tasto sx che fa il semplice drag */
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		//Quindi se sono qui dentro e' perche' sto premendo i tasti giusti
		double xpos, ypos;
		//ricavo le coordinate del mouse x e y rispetto alla finestra
		glfwGetCursorPos(window, &xpos, &ypos);
		tb[curr_tb].mouse_sx_press(xpos, ypos);
	}
	else
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			tb[curr_tb].mouse_sx_release();
		}

}

/* callback function called when a mouse wheel is rotated */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (curr_tb == 0)
		tb[0].mouse_scroll(xoffset, yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	/* every time any key is presse it switch from controlling trackball tb[0] to tb[1] and viceversa */
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
			case GLFW_KEY_0:
				watchCameras = true;
				cameramanFocus = 0;
				break;
			
			case GLFW_KEY_1:
				watchCameras = true;
				cameramanFocus = 1;
				break;
			
			case GLFW_KEY_2:
				watchCameras = true;
				cameramanFocus = 2;
				break;
			
			case GLFW_KEY_3:
				watchCameras = true;
				cameramanFocus = 3;
				break;
			default:
				watchCameras = false;
				break;
		}
	}
}

int main(int argc, char** argv)
{
	//Setuppiamo GLFW---------------------------------------------
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(800, 800, "CarOusel", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glewInit();
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	/* declare the callback functions on mouse events */
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Sincronizza con il refresh rate del monitor

	//Setuppiamo OPENGL---------------------------------------------
	/* define the viewport  */
	glViewport(0, 0, 800, 800);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_MULTISAMPLE);
	glDepthRange(0.01, 1);
	printout_opengl_glsl_info();

	//Carichiamo le risorse (shaders, textures, modelli, matrici...) ---------------------------------------------
	shader basic_shader;
	basic_shader.create_program("shaders/basic.vert", "shaders/basic.frag");
	
	shader depth_shader;
	depth_shader.create_program("shaders/depth.vert", "shaders/depth.frag");
	
	shader depth_lamps_shader;
	depth_lamps_shader.create_program("shaders/depth_lamp.geom", "shaders/depth_lamp.vert", "shaders/depth_lamp.frag");

	gltf_loader gltfLoader;
	/* carico le macchine quindi car_objects */
    gltfLoader.load_to_renderable("assets/models/car1.glb", car, bbox_car);
	/* carico le lamp quindi lamp_objects */
    gltfLoader.load_to_renderable("assets/models/lamp.glb", lamp, bbox_lamp);
	texture lamp_texture = LoadTexture("assets/textures/lampColor.png");
	/* carico gli alberi quindi tree_objects */
	gltfLoader.load_to_renderable("assets/models/trees.glb", tree, bbox_tree);
	/* carico i droni quindi droni_objects */
	gltfLoader.load_to_renderable("assets/models/drone.glb", drone, bbox_drone);
	/* carico i cube per testare */
	gltfLoader.load_to_renderable("assets/models/cube.glb", cube, bbox_cube);
	
	//Setuppa porjection e view matrix
	proj = glm::perspective(glm::radians(45.f), (float)1.0f, 0.1f, 100.f);
	view = glm::lookAt(glm::vec3(0, 1.0f, 1.5f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0, 1.f, 0.f));
	//std::cout << "View Matrix:\n" << glm::to_string(view) << std::endl;
	
	//Setuppa trackball
	tb[0].reset();
	tb[0].set_center_radius(glm::vec3(0, 0, 0), 0.2f);
	curr_tb = 0;

	//Setuppiamo il carosello---------------------------------------------
	race r;
	carousel_loader::load("small_test.svg", "terrain_256.png", r);

	//add 10 cars
	for (int i = 0; i < 10; ++i)
		r.add_car();
	
	//renderable r_track;
	r_track.create();
	game_to_renderable::to_track(r, r_track);
	track_texture = LoadTexture("common/carousel/street_tile.png");


	//renderable r_terrain;
	r_terrain.create();
	game_to_renderable::to_heightfield(r, r_terrain);
	terrain_texture = LoadTexture("assets/textures/grass2.png");

	white = LoadTexture("assets/textures/white.png");
	
	r.start(11, 0, 0, 20);
	r.update();
		//Camera principale
	glm::mat4 mainCamera = glm::lookAt(glm::vec3(0, 1.0f, 1.5f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.0, 1.f, 0.f));

	/* initial light direction */
	Ldir = glm::vec4(0.0, 1.0, 0.0, 0.0);
	k_plane_approx = 0.5;

	/* light projection */
	Lproj.sm_size_x = 512;
	Lproj.sm_size_y = 512;
	Lproj.distance_light = 0.5;
	depth_bias = 0;
	
	ligthDepthFbo.create(Lproj.sm_size_x, Lproj.sm_size_y,true);
	
	//Creo i proiettori per le ombre dei fari
	std::vector<frame_buffer_object> headlightDepthFbos;
	std::vector<headl_light> hlProjectors;

	// Per ogni macchina crea un buffer, 
	for (int i = 0; i < CAR_AMOUNT; i++)
	{
		// Creazione framebuffer
		frame_buffer_object fbo = {};
		fbo.create(1024, 1024, true);
		headlightDepthFbos.push_back(fbo);

		//Creo struttura dati di appoggio per la gestione dei fanali
		headl_light hl = {};
		hl.sm_size_x = 1024;
		hl.sm_size_y = 1024;

		hlProjectors.push_back(hl);
	}


	//Impostazioni luce della scena
	glUseProgram(basic_shader.program);

	//Impostazioni di debug
	basic_shader.SetBool("uSunShadowsEnable", SHADOW_MAPPING);
	basic_shader.SetBool("uCarHeadlightEnable", CAR_HEADLIGHTS);
	
	//Impostazioni luce della scena
	basic_shader.bind("uLampLigthColor");
	basic_shader.SetVector3("uLampLigthColor", glm::vec3(1, 0.97, 0.76));
	basic_shader.bind("uLampC1");
	basic_shader.SetFloat("uLampC1", 0.1f);
	basic_shader.bind("uLampC2");
	basic_shader.SetFloat("uLampC2", 2.0f);
	basic_shader.bind("uLampC3");
	basic_shader.SetFloat("uLampC3", 750.0f);
	check_gl_errors(__LINE__, __FILE__);

	//Carica tutti i lampioni in shader
	basic_shader.bind("uLampsAmount");
	basic_shader.SetInt("uLampsAmount", r.lamps().size());
	check_gl_errors(__LINE__, __FILE__);

	// Imposta il colore dei fari
	basic_shader.bind("uHeadlightColor");
	basic_shader.SetVector3("uHeadlightColor", glm::vec3(1.0f, 0.9f, 0.8f)); // Colore bianco caldo
	check_gl_errors(__LINE__, __FILE__);

	// Imposta la attenuazione
	basic_shader.bind("uHeadlightC1");
	basic_shader.SetFloat("uHeadlightC1", 0.1f); // Costante
	basic_shader.bind("uHeadlightC2");
	basic_shader.SetFloat("uHeadlightC2", 0.02f); // Lineare
	basic_shader.bind("uHeadlightC3");
	basic_shader.SetFloat("uHeadlightC3", 100.f); // Quadratica

	// Angoli del cono (cutoff interno ed esterno)
	basic_shader.bind("uHeadlightCutOff");
	basic_shader.SetFloat("uHeadlightCutOff", glm::cos(glm::radians(5.5f))); // Interno
	basic_shader.bind("uHeadlightOuterCutOff");
	basic_shader.SetFloat("uHeadlightOuterCutOff", glm::cos(glm::radians(50.f))); // Esterno

	// Carica tutti i fari in shader (cars x 2)
	basic_shader.bind("uHeadlightAmount");
	basic_shader.SetInt("uHeadlightAmount", CAR_AMOUNT);
	// Carica tutte le normali dei fari in shader (cars x 2)
	basic_shader.bind("uHeadlightNAmount");
	basic_shader.SetInt("uHeadlightNAmount", CAR_AMOUNT);

	//Impostazioni shadowMapping
	basic_shader.SetVector2("uShadowMapSize", glm::vec2(Lproj.sm_size_x, Lproj.sm_size_y));
	basic_shader.SetFloat("uBias", 0.01f);
	check_gl_errors(__LINE__, __FILE__);

	basic_shader.SetVector2("uHeadMapSize", glm::vec2(hlProjectors[0].sm_size_x, hlProjectors[0].sm_size_y));

	// PROJECTIVE TEXTURE slot 0

	texture headlights_texture = LoadTexturePT("assets/textures/batman.png");
	basic_shader.bind("uHeadlightsTexture");
	BindTexture(basic_shader, "uHeadlightsTexture", headlights_texture, 2);
	/*
	float fov = glm::radians(120.0f), aspectRatio = 0.3f, nearPlane = 0.5f, farPlane = 0.7f;

	glm::mat4 projectionMatrix = glm::perspective(fov, aspectRatio, nearPlane, farPlane);
	basic_shader.bind("uLPProj");
	basic_shader.SetMatrix4x4("uLPProj", projectionMatrix);
	*/
	check_gl_errors(__LINE__, __FILE__);

	glUseProgram(depth_lamps_shader.program);
	depth_lamps_shader.bind("uFarPlane");
	depth_lamps_shader.SetFloat("uFarPlane", 25);

	//Terrain
	stack.load_identity();
	stack.mult(glm::scale(glm::mat4(1), glm::vec3(1/r.bbox().diagonal())));
	glm::vec3 c = r.bbox().center(); c.y = 0;
	stack.mult(glm::translate(glm::mat4(1), -c));

	//Creo i proiettori per le ombre dei lampioni
	/*
	std::vector<point_light> lampProjectors;
	for (int i = 0;  i < r.lamps().size(); i++)
	{
		stick_object l = r.lamps()[i];

		glm::vec3 c = r.bbox().center();
		c.y = 0;
		glm::vec4 lPos = stack.m() * glm::vec4(l.pos + glm::vec3(0, l.height, 0), 1);

		point_light p;
		p.sm_size_x = 128;
		p.sm_size_y = 128;

		p.set(glm::vec3(lPos.x, lPos.y, lPos.z));
		lampProjectors.emplace_back(p);
	}

	std::vector<frame_buffer_object> lampsFbo;
	lampsFbo.reserve(lampProjectors.size());
	//frame_buffer_object lampsFbo[18];
	for (int i = 0;  i < lampProjectors.size(); i++)
	{
		lampsFbo[i].create_fromcubemap(lampProjectors[i].sm_size_x, lampProjectors[i].sm_size_y);
		check_gl_errors(__LINE__, __FILE__, true);
	}
	*/

	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		//glClearColor(0.3f, 0.3f, 0.3f, 1.f);
		check_gl_errors(__LINE__, __FILE__);

		r.update();

		//Aggiorna le matrici dei fanali
		for (int i = 0; i < CAR_AMOUNT; i++)
		{
			glm::mat4 hlMatrix= glm::translate(stack.m() * r.cars()[i].frame, glm::vec3(0, 1.0f, -4.0f));
			glm::mat4 hlView = glm::inverse(hlMatrix);
			hlProjectors[i].set(hlView);
		}

		//Depth Pass --------------------------------------------------------------------------------------
		if(SHADOW_MAPPING){
			//Disegno sul framebuffer la depth dal punto di vista della luce 
			glBindFramebuffer(GL_FRAMEBUFFER, ligthDepthFbo.id_fbo);
			glViewport(0, 0, Lproj.sm_size_x, Lproj.sm_size_y);

			glm::vec3 ligthUp = glm::cross(r.sunlight_direction(), glm::vec3(1, 0, 0));
			Lproj.view_matrix = glm::lookAt(r.sunlight_direction() * Lproj.distance_light, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0, 1, 0));
			Lproj.set_projection(Lproj.view_matrix, 0.5);

			glUseProgram(depth_shader.program);
			depth_shader.SetMatrix4x4("uLightMatrix", Lproj.light_matrix());
			depth_shader.SetFloat("uPlaneApprox", k_plane_approx);
			DrawDepthScene(r, depth_shader);

			//HEADLIGHTS SHADOWMAP
			if (CAR_HEADLIGHTS)
			{
				for (int i = 0; i < CAR_AMOUNT; i++) {


					// Bindo il buffer corrispondente alla macchina i
					glBindFramebuffer(GL_FRAMEBUFFER, headlightDepthFbos[i].id_fbo);
					glViewport(0, 0, hlProjectors[i].sm_size_x, hlProjectors[i].sm_size_y);
					glUseProgram(depth_shader.program);

					// Qui c'e' bisogno della LightMatrix da passare allo shader quindi deve gia' essere calcolato come proj * view
					depth_shader.SetMatrix4x4("uLightMatrix", hlProjectors[i].light_matrix);
					depth_shader.SetFloat("uPlaneApprox", 0.05);

					DrawDepthScene(r, depth_shader);

				}
			}
		}
		check_gl_errors(__LINE__, __FILE__);


		//Rendering Pass --------------------------------------------------------------------------------------
		{
			stack.push();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
			glUseProgram(basic_shader.program);
			basic_shader.SetVector3("uSunDirection", r.sunlight_direction());
			basic_shader.SetVector3("uColor", glm::vec3(1, 1, 1));

			glActiveTexture(GL_TEXTURE0);
    		glBindTexture(GL_TEXTURE_2D, ligthDepthFbo.id_tex);
			basic_shader.SetInt("uShadowMap", 0);
			basic_shader.SetMatrix4x4("uLightMatrix", Lproj.light_matrix());

			//Binda le depth map dei fanali
			if (CAR_HEADLIGHTS) {
				for (int i = 0; i < CAR_AMOUNT; i++)
				{
					basic_shader.bind("uHeadLightMatrix[" + std::to_string(i) + "]");
					basic_shader.SetMatrix4x4("uHeadLightMatrix[" + std::to_string(i) + "]", hlProjectors[i].light_matrix);

					basic_shader.bind("uHeadShadowMap[" + std::to_string(i) + "]");
					BindTextureId(basic_shader, "uHeadShadowMap[" + std::to_string(i) + "]", headlightDepthFbos[i].id_tex, 3+i);
					//BindTextureId(basic_shader, "uTest", headlightDepthFbos[i].id_tex, 2+i);
				}
			}

			glClearColor(0.3f, 0.3f, 0.3f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			if(watchCameras)
			{
				view = glm::inverse((glm::scale(glm::mat4(1), glm::vec3(1/r.bbox().diagonal())) * glm::translate(glm::mat4(1), -c)) * r.cameramen()[cameramanFocus].frame);
			}
			else
			{
				view = mainCamera * tb[0].matrix();
			}

			//Aggiorna camera
			basic_shader.SetMatrix4x4("uProj", proj);
			basic_shader.SetMatrix4x4("uView", view);

			r_terrain.bind();
			basic_shader.SetMatrix4x4("uModel", stack.m());
			BindTexture(basic_shader, "uTexture", terrain_texture, 1);
			glDrawElements(GL_TRIANGLES, 390150, GL_UNSIGNED_INT, 0);

			//Track
			r_track.bind();
			basic_shader.SetMatrix4x4("uModel", stack.m());
			BindTexture(basic_shader, "uTexture", track_texture, 1);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, r_track.vn);

			//Drones
			for (unsigned int ic = 0; ic < r.cameramen().size(); ++ic) {
				stack.push();
				glm::mat4 model = r.cameramen()[ic].frame;
				model = model * glm::scale(glm::mat4(1.f), glm::vec3(0.5f, 0.5f, 0.5f));  // Scala
				model = model * glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 1.0f, 0.0f)); // Traslazione verso l'alto
				stack.mult(model);
				DrawModel(drone, basic_shader, stack.m());
				stack.pop();
			}

			//Trees
			for (int k = 0; k < r.trees().size(); k++) {
				stick_object l = r.trees()[k];
				stack.push();
				stack.mult(glm::scale(glm::translate(glm::mat4(1.0f), l.pos), glm::vec3(0.1f))); // Traslazione

				for (unsigned int i = 0; i < tree.size(); ++i) {
					tree[i].bind();
					stack.push();
					stack.mult(tree[i].transform);
					glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
					BindTexture(basic_shader, "uTexture",white, 1);
					//glBindTexture(GL_TEXTURE_2D, tree[i].mater.base_color_texture);

					// Imposta il colore se necessario
					glm::vec3 objectColor = (i == 1) ? glm::vec3(0.45f, 0.17f, 0.02f) : glm::vec3(0.08f, 0.53f, 0.0f); 
					basic_shader.SetVector3("uColor", objectColor);

					glDrawElements(tree[i]().mode, tree[i]().count, tree[i]().itype, 0);
					stack.pop();
				}
				stack.pop();
			}
			basic_shader.SetVector3("uColor", glm::vec3(1, 1, 1));

			//Lampioni
			lamp[0].bind();
			BindTexture(basic_shader, "uTexture", lamp_texture, 1);
			for (int i = 0;  i < r.lamps().size(); i++)
			{
				stick_object l = r.lamps()[i];
				stack.push();
				stack.mult(glm::scale(glm::translate(glm::mat4(1), l.pos), glm::vec3(0.2)));

				basic_shader.bind("uLampLights[" + std::to_string(i) + "]");
				basic_shader.SetVector3("uLampLights[" + std::to_string(i) + "]", glm::vec3(glm::translate(stack.m(), glm::vec3(0, l.height, 0)) * glm::vec4(0, 0, 0, 1)));

				// Renderizza l'oggetto
				glUniformMatrix4fv(basic_shader["uModel"], 1, GL_FALSE, &stack.m()[0][0]);
				glDrawElements(lamp[0]().mode, lamp[0]().count, lamp[0]().itype, 0);
				stack.pop(); // Ripristina lo stato precedente
			}
/*
*/
			//Cars		
			for (unsigned int k = 0; k < CAR_AMOUNT; ++k) {
				stack.push();
				stack.mult(r.cars()[k].frame);

				// Matrice di rotazione
				glm::mat4 rot = glm::rotate(glm::mat4(1), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				
				// Posizione primo faro 
				glm::vec3 headlightPos1 = glm::vec3(glm::translate(stack.m(), glm::vec3(0, 1.0f, -4.0f)) * glm::vec4(0, 0, 0, 1)); // Cambiata Y e Z
				basic_shader.bind("uHeadlights[" + std::to_string(k) + "]");
				basic_shader.SetVector3("uHeadlights[" + std::to_string(k) + "]", headlightPos1);

				glm::vec3 headlightDir = glm::normalize(glm::vec3(rot * stack.m() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))); // Ruotata di 180° su Y
				basic_shader.bind("uHeadlightN[" + std::to_string(k) + "]");
				basic_shader.SetVector3("uHeadlightN[" + std::to_string(k) + "]", headlightDir);


				// Posizione secondo faro
		/*
				glm::vec3 headlightPos2 = glm::vec3(glm::translate(stack.m(), glm::vec3(-0.7f, .8f, -1.7f)) * glm::vec4(0, 0, 0, 1)); // Cambiata Y e Z
				basic_shader.bind("uHeadlights[" + std::to_string(k * 2 + 1) + "]");
				basic_shader.SetVector3("uHeadlights[" + std::to_string(k * 2 + 1) + "]", headlightPos2);

				// Direzione dei fari (corretta per i fari sul cofano)
				glm::vec3 headlightDir = glm::normalize(glm::vec3(rot * stack.m() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))); // Ruotata di 180° su Y
				basic_shader.bind("uHeadlightN[" + std::to_string(k * 2) + "]");
				basic_shader.SetVector3("uHeadlightN[" + std::to_string(k * 2) + "]", headlightDir);

				basic_shader.bind("uHeadlightN[" + std::to_string(k * 2 + 1) + "]");
				basic_shader.SetVector3("uHeadlightN[" + std::to_string(k * 2 + 1) + "]", headlightDir);

				stack.mult(glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f)));
				stack.mult(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)));

	
				glm::mat4 viewMatrix = glm::inverse((glm::scale(glm::mat4(1), glm::vec3(1/r.bbox().diagonal())) * glm::translate(glm::mat4(1), -c)) * r.cars()[k].frame);

				/*
				basic_shader.bind("uProjectorPos[" + std::to_string(k) + "]");
				basic_shader.SetVector3("uProjectorPos[" + std::to_string(k) + "]", headlightPos1);
				
				basic_shader.bind("uProjectorDir[" + std::to_string(k) + "]");
				basic_shader.SetVector3("uProjectorDir[" + std::to_string(k) + "]", headlightDir);

				basic_shader.bind("uLPView[" + std::to_string(k) + "]");
				basic_shader.SetMatrix4x4("uLPView[" + std::to_string(k) + "]", viewMatrix);
				*/

				stack.mult(glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f)));
				stack.mult(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f)));

				DrawModel(car, basic_shader, stack.m());
				stack.pop();
			}


			stack.pop();
		}
		check_gl_errors(__LINE__, __FILE__);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glUseProgram(0);
	glfwTerminate();
	return 0;
}
