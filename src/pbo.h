#pragma once

typedef unsigned int GLuint;
class streamer;

class pbo {

public:
	explicit pbo(streamer* e);
	~pbo();

	void fill(unsigned char* src);
	void draw();

	GLuint tex_id;

    void toggle_texture_filtering() const;
private:
    streamer* eng;
    
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;
    
    GLuint vbo_id;
    GLuint vao_id;
    GLuint uv_id;

	GLint mvp_loc;
	GLint tex_loc;

	GLuint pbo_ids[2];
	int pbo_i;
	uint8_t* buffers_data;
	uint8_t* buffers[2];
};

