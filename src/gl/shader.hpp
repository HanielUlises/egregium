#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

namespace gl {

// RAII-owning shader program. Move-only (owns a GL program name).
class Shader {
public:
    Shader() = default;
    ~Shader();
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Compiles + links; throws std::runtime_error (with the GL info log) on failure.
    static Shader fromSource(const std::string& vertSrc, const std::string& fragSrc);

    void use() const;
    GLuint id() const { return program_; }
    bool valid() const { return program_ != 0; }

    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setFloat(const char* name, float v) const;
    void setInt(const char* name, int v) const;

private:
    explicit Shader(GLuint program) : program_(program) {}
    GLuint program_ = 0;
    GLint loc(const char* name) const;
};

} // namespace gl
