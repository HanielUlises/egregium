#include "gl/shader.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

namespace gl {

namespace {
GLuint compile(GLenum type, const std::string& src) {
    GLuint id = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(id, 1, &csrc, nullptr);
    glCompileShader(id);
    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len) + 1, '\0');
        glGetShaderInfoLog(id, len, nullptr, log.data());
        glDeleteShader(id);
        throw std::runtime_error(std::string("shader compile error: ") + log.data());
    }
    return id;
}
} // namespace

Shader Shader::fromSource(const std::string& vertSrc, const std::string& fragSrc) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len) + 1, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error(std::string("shader link error: ") + log.data());
    }
    return Shader(prog);
}

Shader::~Shader() {
    if (program_) glDeleteProgram(program_);
}
Shader::Shader(Shader&& o) noexcept : program_(o.program_) { o.program_ = 0; }
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (program_) glDeleteProgram(program_);
        program_ = o.program_;
        o.program_ = 0;
    }
    return *this;
}

void Shader::use() const { glUseProgram(program_); }
GLint Shader::loc(const char* name) const { return glGetUniformLocation(program_, name); }
void Shader::setMat4(const char* name, const glm::mat4& m) const { glUniformMatrix4fv(loc(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setVec3(const char* name, const glm::vec3& v) const { glUniform3fv(loc(name), 1, glm::value_ptr(v)); }
void Shader::setFloat(const char* name, float v) const { glUniform1f(loc(name), v); }
void Shader::setInt(const char* name, int v) const { glUniform1i(loc(name), v); }

} // namespace gl
