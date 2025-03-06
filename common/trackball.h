#ifndef MATERIAL_track_H
#define MATERIAL_track_H
#include <glm/glm.hpp>  
#include <glm/ext.hpp>  
#include <glm/gtx/string_cast.hpp>

class trackball {

public: trackball() { reset(); old_tb_matrix = glm::mat4(1.f);}

private:
	/* a bool variable that indicates if we are currently rotating the trackball*/
	bool is_trackball_dragged;

	/* booleani necessari alla traslazione della scena */
	bool is_dragging;
	double prev_xpos, prev_ypos;

	/* a bool variable that indicates if the transformation has been changed since last time it was check*/
	bool changed;

	/* p0 and p1 points on the sphere */
	glm::vec3 p0, p1;

	/* matrix to transform the scene according to the trackball: rotation only*/
	glm::mat4 rotation_matrix;

	/* matrix to transform the scene according to the trackball: scaling only*/
	glm::mat4  scaling_matrix;

	/* matrix to transform the scene according to the trackball: translation only*/
	glm::mat4  translation_matrix;

	/* old trackball*/
	glm::mat4  old_tb_matrix;

	float scaling_factor;


	/* trackball center */
	glm::vec3 center;

	/* trackball radius */
	float radius;


	/* transform from viewport to the view reference frame */
	void viewport_to_ray(glm::mat4 proj, double pX, double pY, glm::vec4  &p0, glm::vec4 &  d) {
		GLint vp[4];
		glm::mat4 proj_inv = glm::inverse(proj);
		glGetIntegerv(GL_VIEWPORT, vp);
		glm::vec4 p1;
		p1.x = p0.x = -1.f + ((float)pX / vp[2])  * (1.f - (-1.f));
		p1.y = p0.y = -1.f + ((vp[3] - (float)pY) / vp[3]) * (1.f - (-1.f));
		p0.z = -1;
		p1.z = 1;
		p1.w = p0.w = 1.0;
		p0 = proj_inv*p0; p0 /= p0.w;
		p1 = proj_inv*p1; p1 /= p1.w;
		d = glm::normalize(p1 - p0);
	}
	/* handles the intersection between the position under the mouse and the sphere.
	*/
	bool cursor_sphere_intersection(glm::mat4 proj, glm::mat4 view, glm::vec3 & int_point, double xpos, double ypos) {
		glm::mat4 view_frame = glm::inverse(view);

		glm::vec4 o,d;
		viewport_to_ray(proj, xpos, ypos,o ,d );

		o  = view_frame* o  ;
		d  = view_frame* d  ;

		bool hit = intersection_ray::sphere(int_point, o, d, center, radius);
		if (hit)
			int_point -= center;

		/* this was left to "return true" in class.. It was a gigantic bug with almost never any consequence, except while
		click near the silohuette of the sphere.*/
		return hit;
	}

public:
	void reset() {
		scaling_factor = 1.f;
		scaling_matrix = glm::mat4(1.f);
		rotation_matrix = glm::mat4(1.f);
		translation_matrix = glm::mat4(1.f);
	}
	void set_center_radius(glm::vec3 c, float r) {
		old_tb_matrix = this->matrix();
		reset();
		center = c;
		radius = r;
		translation_matrix =   glm::translate(glm::mat4(1.f), center);
	}

	/*metodo della classe trackball, vediamo.... Quello che succede e'
	che e' prende la matrice di proiezione e di vista, e le coordinate del mouse
	a quel punto controlla, se c'e' stato uno spostamento quindi il predicato is_trackball_dragged
	allora ricalcola il cursor_sphere_intersection e via
	*/
	
	void mouse_move(glm::mat4 proj, glm::mat4 view, double xpos, double ypos) {
		
		if (is_trackball_dragged) {

			if (cursor_sphere_intersection(proj, view, p1, xpos, ypos)) {
				glm::vec3 rotation_vector = glm::cross(glm::normalize(p0), glm::normalize(p1));

				/* avoid near null rotation axis*/
				if (glm::length(rotation_vector) > 0.01) {
					float alpha = glm::asin(glm::length(rotation_vector));
					glm::mat4 delta_rot = glm::rotate(glm::mat4(1.f), alpha, rotation_vector);
					rotation_matrix = delta_rot * rotation_matrix;

					/*p1 becomes the p0 value for the next movement */
					p0 = p1;
				}
			}
		}
		
		if (is_dragging) {

			double dx = xpos - prev_xpos;
			double dy = ypos - prev_ypos;

			float translation_factor_x = (float)dx * 0.001f;
			float translation_factor_y = (float)dy * 0.001f;
			translation_matrix = glm::translate(translation_matrix, glm::vec3(translation_factor_x,-translation_factor_y, 0.0f));
		
			prev_xpos = xpos;
			prev_ypos = ypos;

			changed = true;
		}
	}

	void mouse_sx_press(double xpos, double ypos) {
	//in questa funzione devo quando tengo premuto il tasto sx spostare la visuale quindi il mio view frame
	//sulle coordinate del mouse, quindi quello che voglio fare e' prendere le coordinate del mouse e moltiplicarle (?)
		is_dragging = true;
		prev_xpos = xpos;
		prev_ypos = ypos;
		//avviso che la scena e' cambiata
		changed = true;
	}
	void mouse_sx_release() {
		is_dragging = false;
	}

	void mouse_middle_press(glm::mat4 proj, glm::mat4 view, double xpos, double ypos) {
		glm::vec3 int_point;
		if (cursor_sphere_intersection(proj, view, int_point, xpos, ypos)) {
			p0 = int_point;
			is_trackball_dragged = true;
		}
	}

	void mouse_middle_release() {
		is_trackball_dragged = false;
	}

	void mouse_scroll(double xoffset, double yoffset)
	{
		changed = true;
		scaling_factor *= (float)((yoffset>0) ? 1.1 : 0.97);
		scaling_matrix = glm::scale(glm::mat4(1.f), glm::vec3(scaling_factor, scaling_factor, scaling_factor));
	}

	glm::mat4 matrix() {
		return translation_matrix*scaling_matrix* rotation_matrix*glm::inverse(translation_matrix)*this->old_tb_matrix;
	}

	bool is_moving() {
		return is_trackball_dragged;
	}

	bool is_changed() {
		if (changed || is_trackball_dragged) {
			changed = false;
			return true;
		}
		return false;
	}
};

#endif