#pragma once

class streamer;

class pbo {

public:
	explicit pbo(streamer* e);
	~pbo();

	void fill(unsigned char* src);
	void draw();

	uint tex_id;

    void toggle_texture_filtering() const;
private:
    streamer* eng;
    
    uint32_t vertex_buffer, vertex_shader, fragment_shader, program;
    int32_t mvp_location, vpos_location, vcol_location;
    
    uint32_t vbo_id;
    uint32_t vao_id;
    uint32_t uv_id;

	int32_t mvp_loc;
    int32_t tex_loc;

	uint32_t pbo_ids[2];
	int pbo_i;
	uint8_t* buffers_data;
	uint8_t* buffers[2];
};

