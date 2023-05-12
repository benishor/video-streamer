#include <glad/glad.h>
#include <vector>
#include <iostream>
#include <cstring>
#include "streamer.h"
#include "pbo.h"
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//geometry: textured quad
const float quad[] = {
        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, -1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 1.0f};

const float texcoord[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f};
//        0.0f, 1.0f,
//        0.0f, 0.0f,
//        1.0f, 0.0f,
//        1.0f, 0.0f,
//        1.0f, 1.0f,
//        0.0f, 1.0f};

const char *vertex_shader_text =
        "#version 330 core\n"
        "layout(location = 0) in vec4 pos;\n"
        "layout(location = 1) in vec2 tex;\n"
        "smooth out vec2 UV;\n"
        "uniform mat4 MVP;\n"
        "void main() {\n"
        "  gl_Position = MVP * pos;\n"
        "  UV = tex;\n"
        "}";

const char *fragment_shader_text =
        "#version 330 core\n"
        "smooth in vec2 UV;\n"
        "out vec4 outColor;\n"
        "uniform sampler2D cltexture;\n"
        "void main() {\n"
        "  outColor = vec4(texture(cltexture, UV).rgb, 1.0);\n"
        "}";


GLuint create_program(const char *vertexSrc,
                      const char *fragmentSrc);

pbo::pbo(streamer *e, int w, int h) : eng(e), width(w), height(h) {

    //buffers
    glGenVertexArrays(1, &vao_id);
    glBindVertexArray(vao_id);

    //geometry buffer
    glGenBuffers(1, &vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), &quad[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //texture coordinate buffer
    GLuint uv_id;
    glGenBuffers(1, &uv_id);

    glBindBuffer(GL_ARRAY_BUFFER, uv_id);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), &texcoord[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    buffers_data = new uint8_t[width * height * 3 * 2];
    buffers[0] = &buffers_data[0];
    buffers[1] = &buffers_data[width * height * 3];

    // create texture
    std::vector<char> tc(width * height * 3, 0);
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 width,
                 height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    program = create_program(vertex_shader_text, fragment_shader_text);
    glUseProgram(program);

    mvp_loc = glGetUniformLocation(program, "MVP");
    tex_loc = glGetUniformLocation(program, "cltexture");

    // only need texture unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glUniform1i(tex_loc, 0);


    // generate pbos
    glGenBuffers(2, pbo_ids);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_ids[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 3, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_ids[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 3, nullptr, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    pbo_i = 0;
}

pbo::~pbo() {
    free(buffers_data);
    glDeleteBuffers(2, pbo_ids);
    glDeleteBuffers(1, &vbo_id);
    glDeleteBuffers(1, &uv_id);
    glDeleteVertexArrays(1, &vao_id);
    glDeleteTextures(1, &tex_id);
}


void pbo::draw() {
    glUseProgram(program);
    {
        glm::mat4 mvp = glm::mat4(1.0f);
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));

        // select geometry
        glBindVertexArray(vao_id);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        //draw
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // unbind
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    glUseProgram(0);
}


void pbo::fill(unsigned char *src) {
    if (src == nullptr) return;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_ids[pbo_i]);
    {
        // send to texture
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    //glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    pbo_i = (pbo_i + 1) % 2;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_ids[pbo_i]);
    {
        auto mapped_buffer = (unsigned char *) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        memcpy(mapped_buffer, src, width * height * 3);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    }
    //glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void pbo::toggle_texture_filtering() const {
    glBindTexture(GL_TEXTURE_2D, tex_id);

    GLint param;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &param);

    if (param == GL_NEAREST) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
}

GLuint create_program(const char *vertexSrc,
                      const char *fragmentSrc) {
    // Create the shaders
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    GLint res = GL_FALSE;
    int logsize = 0;
    // Compile Vertex Shader
    glShaderSource(vs, 1, &vertexSrc, 0);
    glCompileShader(vs);

    // Check Vertex Shader
    glGetShaderiv(vs, GL_COMPILE_STATUS, &res);
    glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &logsize);

    if (logsize > 1) {
        std::vector<char> errmsg(logsize + 1, 0);
        glGetShaderInfoLog(vs, logsize, 0, &errmsg[0]);
        std::cout << &errmsg[0] << std::endl;
    }
    // Compile Fragment Shader
    glShaderSource(fs, 1, &fragmentSrc, 0);
    glCompileShader(fs);

    // Check Fragment Shader
    glGetShaderiv(fs, GL_COMPILE_STATUS, &res);
    glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &logsize);
    if (logsize > 1) {
        std::vector<char> errmsg(logsize + 1, 0);
        glGetShaderInfoLog(fs, logsize, 0, &errmsg[0]);
        std::cout << &errmsg[0] << std::endl;
    }

    // Link the program
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    // Check the program
    glGetProgramiv(program, GL_LINK_STATUS, &res);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logsize);
    if (logsize > 1) {
        std::vector<char> errmsg(logsize + 1, 0);
        glGetShaderInfoLog(program, logsize, 0, &errmsg[0]);
        std::cout << &errmsg[0] << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}
