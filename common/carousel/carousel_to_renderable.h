#ifndef MATERIAL_carr_H
#define MATERIAL_carr_H

#include "../renderable.h"
#include "carousel.h"



struct game_to_renderable {
	
	static void ct(float* dst, glm::vec3 src) {
		dst[0] = src.x;
		dst[1] = src.y;
		dst[2] = src.z;
	}

	static glm::vec3 hf_point(const race& r, int x, int z)
	{
		terrain ter = r.ter();
		const unsigned int& Z =static_cast<unsigned int>(r.ter().size_pix[1]);
		const unsigned int& X =static_cast<unsigned int>(r.ter().size_pix[0]);

		glm::vec3 p = glm::vec3
		(
			ter.rect_xz[0] + (x / float(X)) * ter.rect_xz[2],
			r.ter().hf(x, z),
			ter.rect_xz[1] + (z / float(Z)) * ter.rect_xz[3]
		);

		return p;
	}

	static void to_track(const race& r, renderable& r_t) {
		std::vector<float> buffer_pos;
		std::vector<float> buffer_norm;
		buffer_pos.resize(r.t().curbs[0].size() * 2 * 3); // Ogni punto ha due vertici (due lati della pista) e ciascun vertice ha 3 coordinate (x, y, z)
		buffer_norm.resize(r.t().curbsNormals[0].size() * 2 * 3); 
		std::vector<float> uv_coords;
		float total_distance = 0.0f;
		std::vector<float> distances;

		// Calcoliamo la distanza totale lungo la pista
		for (unsigned int i = 1; i < r.t().curbs[0].size(); ++i) {
			float dx = r.t().curbs[0][i].x - r.t().curbs[0][i-1].x;
			float dy = r.t().curbs[0][i].y - r.t().curbs[0][i-1].y;
			// l'insieme di tutte le diagonali
			float segment_distance = sqrt(dx * dx + dy * dy);
			total_distance += segment_distance;
			distances.push_back(total_distance);
		}
		
		// Lo ripeto per ogni coppia di vertici
		float repeat_factor = 25.0f;

		// Genera le coordinate della pista e le UV
		for (unsigned int i = 0; i < r.t().curbs[0].size(); ++i) {
			// Vertice sinistro
			ct(&buffer_pos[(2 * i) * 3], r.t().curbs[0][i % r.t().curbs[0].size()]);
			// Vertice destro
			ct(&buffer_pos[(2 * i + 1) * 3], r.t().curbs[1][i % r.t().curbs[1].size()]);

			// Vertice sinistro
			ct(&buffer_norm[(2 * i) * 3], r.t().curbsNormals[0][i % r.t().curbsNormals[0].size()]);
			// Vertice destro
			ct(&buffer_norm[(2 * i + 1) * 3], r.t().curbsNormals[1][i % r.t().curbsNormals[1].size()]);

			/* Qui normalizzo la distanza, piu' grande e' dx e piu' ripeto */
			float v = (distances[i] / total_distance) * repeat_factor;

			uv_coords.push_back(0.0f);
			uv_coords.push_back(v);

			uv_coords.push_back(1.0f);
			uv_coords.push_back(v);
		}

		// Aggiungi i dati dei vertici e delle UV al buffer
		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3);
		r_t.add_vertex_attribute<float>(&buffer_norm[0], static_cast<unsigned int>(buffer_norm.size()), 2, 3);
		r_t.add_vertex_attribute<float>(&uv_coords[0], static_cast<unsigned int>(uv_coords.size()), 4, 2);
	}

	static void to_stick_object(const std::vector<stick_object>& vec, renderable& r_t) {

		std::vector<float> buffer_pos;
		buffer_pos.resize((vec.size()*2) * 3 );
		for (unsigned int i = 0; i < vec.size();++i) {
			ct(&buffer_pos[(2 * i) * 3], vec[i].pos);
			ct(&buffer_pos[(2 * i+1) * 3], vec[i].pos+glm::vec3(0, vec[i].height,0));
		}

		r_t.add_vertex_attribute<float>(&buffer_pos[0], static_cast<unsigned int>(buffer_pos.size()), 0, 3);
	}

	static void to_tree(const race& r, renderable& r_t) {
		to_stick_object(r.trees(), r_t);
	}
	static void to_lamps(const race& r, renderable& r_t) {
		to_stick_object(r.lamps(), r_t);
	}

	static void to_heightfield(const race& r, renderable& r_hf) {
		std::vector<unsigned int > buffer_id;
		const unsigned int& Z =static_cast<unsigned int>(r.ter().size_pix[1]);
		const unsigned int& X =static_cast<unsigned int>(r.ter().size_pix[0]);

		terrain ter = r.ter();

		std::vector<float>   hf3d;
		for (unsigned int iz = 0; iz < Z; ++iz)
			for (unsigned int ix = 0; ix < X; ++ix) {
				hf3d.push_back(ter.rect_xz[0] + (ix / float(X)) * ter.rect_xz[2]);
				hf3d.push_back(r.ter().hf(ix, iz));
				hf3d.push_back(ter.rect_xz[1] + (iz / float(Z)) * ter.rect_xz[3]);
			
			}
		
		std::vector<float>   normals;
		for (unsigned int iz = 0; iz < Z; ++iz)
			for (unsigned int ix = 0; ix < X; ++ix)
			{
				
				glm::vec3 p = hf_point(r, ix, iz);

				glm::vec3 right = p + glm::vec3(1, 0, 0); 
				glm::vec3 forward = p + glm::vec3(0, 0, 1);

				if(iz < Z-1)
				{
					forward = hf_point(r, ix, iz+1);
				}

				if(ix < X-1)
				{
					right = hf_point(r, ix+1, iz);
				}

				glm::vec3 normal = glm::normalize(glm::cross((forward-p), (right-p)));

				normals.push_back(normal.x);
				normals.push_back(normal.y);
				normals.push_back(normal.z);
			}
		
		std::vector<float> uv_coords;
		for (unsigned int iz = 0; iz < Z; ++iz) {
			for (unsigned int ix = 0; ix < X; ++ix) {
				// Le coordinate UV per ogni vertice sono basate sulla posizione sulla griglia
				float u = static_cast<float>(ix) / (X - 1) * 5.0f;  // Normalizzazione tra 0 e 1 per l'asse X
				float v = static_cast<float>(iz) / (Z - 1) * 5.0f;  // Normalizzazione tra 0 e 1 per l'asse Z

				uv_coords.push_back(u);  // Aggiungi la coordinata u
				uv_coords.push_back(v);  // Aggiungi la coordinata v
			}
		}


		for (unsigned int iz = 0; iz < Z-1; ++iz)
			for (unsigned int ix = 0; ix < X-1; ++ix) {
				
				buffer_id.push_back((iz * Z) + ix);
				buffer_id.push_back((iz * Z) + ix + 1);
				buffer_id.push_back((iz + 1) * Z + ix + 1);

				buffer_id.push_back((iz * Z) + ix);
				buffer_id.push_back((iz + 1) * Z + ix + 1);
				buffer_id.push_back((iz + 1) * Z + ix);
			}

		std::cout << "buffer_id.size() = " << buffer_id.size() << std::endl;
		r_hf.add_vertex_attribute<float>(&hf3d[0], X * Z * 3, 0, 3);
		r_hf.add_vertex_attribute<float>(&normals[0],  X * Z * 3, 2, 3);
		r_hf.add_vertex_attribute<float>(&uv_coords[0],  X * Z * 2, 4, 2);
		r_hf.add_indices<unsigned int>(&buffer_id[0], static_cast<unsigned int>(buffer_id.size()), GL_TRIANGLES);        

    }

};
#endif
