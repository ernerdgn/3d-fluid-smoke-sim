#include "GpuGrid3D.h"
#include <iostream>
#include <vector>

GLuint GpuGrid3D::create3DTexture(int internal_format, int format)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_3D, textureID);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // alloc, fill nullptr
    glTexImage3D(GL_TEXTURE_3D, 0, internal_format, m_width, m_height, m_depth,
        0, format, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return textureID;
}

GpuGrid3D::GpuGrid3D(int width, int height, int depth)
    : m_width(width), m_height(height), m_depth(depth)
{
    // density
    m_densityTexA = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_densityTexB = create3DTexture(GL_RGBA32F, GL_RGBA);

    // velo
    m_velocityTexA = create3DTexture(GL_RGBA32F, GL_RGBA);
    m_velocityTexB = create3DTexture(GL_RGBA32F, GL_RGBA);
}

GpuGrid3D::~GpuGrid3D()
{
    GLuint textures[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB
    };
    glDeleteTextures(4, textures);
}

void GpuGrid3D::clear(Shader& clearComputeShader)
{
    clearComputeShader.use();

    GLuint workGroupsX, workGroupsY, workGroupsZ;
    getWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    GLuint texturesToClear[] = {
        m_densityTexA, m_densityTexB,
        m_velocityTexA, m_velocityTexB
    };

    for (GLuint tex : texturesToClear)
    {
        // bind tex to img unit0
        glBindImageTexture(0, tex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        // launch 16x16x16 = 4096 work groups
        // 4096 * 512 = 2 097 152 threads (one for each 3D pixel)
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // finish write
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}

void GpuGrid3D::swapDensityBuffers()
{
    std::swap(m_densityTexA, m_densityTexB);
}

void GpuGrid3D::swapVelocityBuffers()
{
    std::swap(m_velocityTexA, m_velocityTexB);
}

void GpuGrid3D::step(Shader& splatShader,
    const glm::vec3& mouse_pos3D, const glm::vec3& mouse_vel,
    bool is_bouncing)
{
    splatShader.use();

    GLuint workGroupsX, workGroupsY, workGroupsZ;
    getWorkGroups(workGroupsX, workGroupsY, workGroupsZ);

    float brush_radius = m_width * 0.025f; // 2.5% of the obj

    // set const uniforms
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_brush_center3D"), 1, glm::value_ptr(mouse_pos3D));
    glUniform1f(glGetUniformLocation(splatShader.ID, "u_radius"), brush_radius);
    glUniform1i(glGetUniformLocation(splatShader.ID, "u_is_bouncing"), is_bouncing ? 1 : 0);

    // splat velo
    glUniform3fv(glGetUniformLocation(splatShader.ID, "u_force"), 1, glm::value_ptr(mouse_vel));

    // bind textures to img units - 0 (read) / 1 (write)
    glBindImageTexture(0, m_velocityTexA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, m_velocityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // launch comp shader
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // give it a moment

    swapVelocityBuffers(); // res->texA

    // splat dens
    glUniform3f(glGetUniformLocation(splatShader.ID, "u_force"), 1.0f, 0.0f, 0.0f);

    glBindImageTexture(0, m_densityTexA, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA32F);
    glBindImageTexture(1, m_densityTexB, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // *elevator music plays*

    swapDensityBuffers(); // res->texA

    // TODO: ADV, DIFFUSE, PROJ

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GpuGrid3D::getWorkGroups(GLuint& groupsX, GLuint& groupsY, GLuint& groupsZ)
{
    // total group = total size / group size
    groupsX = (m_width + 7) / 8;  // (128 + 7) / 8 ~ 16
    groupsY = (m_height + 7) / 8; // (128 + 7) / 8 ~ 16
    groupsZ = (m_depth + 7) / 8;  // (128 + 7) / 8 ~ 16
}